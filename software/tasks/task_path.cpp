#include "mda_tasks.h"
#include "mda_vision.h"

#define GATE_START_DEPTH 25
#define GATE_FORWARD_SPEED 5
#define GATE_ATTITUDE_CHECK_DELAY 50
#define PATH_SEARCH_SPEED 1

// Global declarations
const int PATH_DELTA_DEPTH = 50;
const int MASTER_TIMEOUT = 120;
const int ALIGN_DELTA_DEPTH = 0;

enum TASK_STATE {
    STARTING_GATE,
    STARTING_PATH,
    AT_SEARCH_DEPTH,
    AT_ALIGN_DEPTH
};

MDA_TASK_PATH::MDA_TASK_PATH (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
    pix_x_old = pix_y_old = 0;
}

MDA_TASK_PATH::~MDA_TASK_PATH ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_PATH::run_task(){
    puts("Press q to quit");

    MDA_VISION_MODULE_PATH path_vision;
    MDA_VISION_MODULE_GATE gate_vision;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;
    
    TASK_STATE state = STARTING_GATE;
    bool done_gate = false;
    bool done_path = false;

    // read the starting orientation
    int starting_yaw = attitude_input->yaw();
    printf("Starting yaw: %d\n", starting_yaw);
    
    int GATE_DEPTH = attitude_input->depth() + GATE_START_DEPTH;
    set (DEPTH, GATE_DEPTH);
    
    /*
    read_mv_setting ("hacks.csv", "GATE_DEPTH", GATE_DEPTH);
    printf("Rose: Going to depth: %d\n", GATE_DEPTH);
    set (DEPTH, GATE_DEPTH/2);
    set (DEPTH, GATE_DEPTH/4*3);
    set (DEPTH, GATE_DEPTH);
    //set(DEPTH, 100);
    */

    // go to the starting orientation in case sinking changed it
    set (YAW, starting_yaw);
    
    // counter to check depth and yaw
    int counter = 0;

    //TIMER master_timer;
    TIMER timer;
    timer.restart();

    while (1) {

        IplImage* frame = NULL;
        MDA_VISION_RETURN_CODE vision_code = NO_TARGET;
        MDA_VISION_RETURN_CODE gate_vision_code = NO_TARGET;
        (void) gate_vision_code;
        /*if (!done_gate) {
            frame = image_input->get_image(FWD_IMG);
            if (!frame) {
                ret_code = TASK_ERROR;
                break;
            }
            gate_vision_code = gate_vision.filter(frame);
        }*/

        frame = image_input->get_image(DWN_IMG);
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }
        vision_code = path_vision.filter(frame);
        
        // clear fwd image. RZ - do we need this?
        // This ensures the other camera is properly logged
        // and that the webcam cache is cleared so it stays in sync - VZ
        // image_input->ready_image(FWD_IMG);

        /**
        * Basic Algorithm:
        *  - Go to path
        *  - Align with path
        */

        printf("Rose: current depth is: %d\n", attitude_input->depth());
        printf("Rose: current yaw is: %d\n", attitude_input->yaw());
        if (!done_gate) {
            if (state == STARTING_GATE) {
                printf("Starting Gate: Moving Foward at High Speed\n");
                set(SPEED, GATE_FORWARD_SPEED);
		if (counter % GATE_ATTITUDE_CHECK_DELAY == 0)
        	{
            	    set (DEPTH, GATE_DEPTH);
            	    printf("Adam: set depth to %d\n", GATE_DEPTH);
            	    set (YAW, starting_yaw);
            	    printf("Rose: set yaw = %d\n", starting_yaw);
            	    counter = 0;
		}
		counter++;

                if (timer.get_time() > MASTER_TIMEOUT) {
                    printf ("Starting Gate: Master Timer Timeout!!\n");
                    return TASK_MISSING;
                }

                else if (gate_vision_code == FULL_DETECT) {
                    printf ("Starting Gate: Full Detect\n");
                    int ang_x = gate_vision.get_angular_x();
                    set_yaw_change(ang_x);

                    if (gate_vision.get_range() < 420) { // finished the gate
                        printf ("Range = %d, Approaching Gate\n", gate_vision.get_range());
                        timer.restart();
                        while (timer.get_time() < 3) {
                            set(SPEED, 6);
                        }
                        set(SPEED, 0);
                        printf ("Gate Task Done!!\n");

                        // get ready for path task
                        done_gate = true;
                        set(YAW, starting_yaw);
                        state = STARTING_PATH;
                    }

                    timer.restart();
                }

                // if path vision saw something, go do the path task
                if (vision_code != NO_TARGET) {
                    printf ("\nSaw Path! Going to Path vision!");
                    done_gate = true;
                    set(SPEED, 0);
                    set(YAW, starting_yaw);
                    timer.restart();
                    while (timer.get_time() < 2);
                    state = STARTING_PATH;
                }
            }
        }


        else if (!done_path) {
            // calculate some values that we will need
            float xy_ang = path_vision.get_angular_x(); // angle equal to atan(x/y)
            float pos_angle = path_vision.get_angle();  // PA, equal to orientation of the thing
            int pix_x = path_vision.get_pixel_x();
            int pix_y = path_vision.get_pixel_y();
            int pix_distance = sqrt(pow(pix_y,2) + pow(pix_x,2));

            printf("xy_distance = %d    xy_angle = %5.2f\n==============================\n", pix_distance, pos_angle);

            if (state == STARTING_PATH) {
                if (vision_code == NO_TARGET) {
                    printf ("Starting: No target\n");
                    set(SPEED, PATH_SEARCH_SPEED);
                    if (timer.get_time() > MASTER_TIMEOUT) { // timeout
                        printf ("Master Timeout\n");
                        return TASK_MISSING;
                    }
                }
                else if (vision_code == UNKNOWN_TARGET) {
                    printf ("Starting: Unknown target\n");
                    timer.restart();
                }
                else {
                    printf ("Starting: Good\n");
                    set(SPEED, 0);
                    timer.restart();
                    state = AT_SEARCH_DEPTH;
                }
            }
            else if (state == AT_SEARCH_DEPTH){
                if (vision_code == NO_TARGET) {
                    if (timer.get_time() < 1) {
                        continue;
                    }
                    printf ("Searching: No target\n");
                    if (timer.get_time() > 5) { // timeout, go back to starting state
                        printf ("Timeout\n");
                        //set (YAW, starting_yaw);
                        timer.restart();
                        path_vision.clear_frames();
                        state = STARTING_PATH;
                    }
                    else if (timer.get_time() % 2 == 0) { // spin around a bit to try to re-aquire?
                        //move (RIGHT, 45);
                    }
                    else { // just wait
                    }
                }
                else if (vision_code == UNKNOWN_TARGET) {
                    printf ("Searching: Unknown target\n");
                    timer.restart();
                }
                else {
                    timer.restart();
                    printf ("Searching: Good\n");
                    if(pix_distance > frame->height/5){ // move over the path
                        if (abs(xy_ang) < 10) {
                            // go fowards or backwards depending on the pix_y value 
                            if (pix_y >= 0) { 
                                printf ("Set speed foward\n");
                                set(SPEED, 3);
                            } else { 
                                printf ("Set speed reverse\n");
                                set(SPEED, -3);
                            }
                        }
                        else {
                            if (abs(xy_ang) > 90 ) {
                                // turn different direction based on pix_y value
                                xy_ang = (xy_ang > 0) ? xy_ang - 180 : xy_ang + 180; 
                            } 
                            printf("Turning %s %d degrees (xy_ang)\n", (abs(xy_ang) > 0) ? "Right" : "Left", static_cast<int>(abs(xy_ang)));
                            set(SPEED, 0);
                            move(RIGHT, xy_ang);
                            path_vision.clear_frames();
                        }
                    }
                    else {                              // we are over the path, sink and try align state
                        set(SPEED, 0);
                        move(SINK, ALIGN_DELTA_DEPTH);
                        timer.restart();
                        path_vision.clear_frames();
                        state = AT_ALIGN_DEPTH;
                    }
                }   
            }
            else if (state == AT_ALIGN_DEPTH) {
                if (vision_code == NO_TARGET) {     // wait for timeout
                    printf ("Aligning: No target\n");
                    if (timer.get_time() > 6) { // timeout
                        printf ("Timeout\n");
                        move(RISE, ALIGN_DELTA_DEPTH);
                        timer.restart();
                        path_vision.clear_frames();
                        state = AT_SEARCH_DEPTH;
                    }
                }
                else if (vision_code == UNKNOWN_TARGET) {
                    printf ("Aligning: Unknown target\n");
                    timer.restart();
                }
                else {
                    if (abs(pos_angle) >= 10) {
                        move(RIGHT, pos_angle);
                        path_vision.clear_frames();
                        timer.restart();
                    }
                    else {
                        done_path = true;
                    }
                }
            }
        } // done_path
        else {
            // wait foward 2 secs, then charge foward for 2 secs
            timer.restart();
            while (timer.get_time() < 2) {
                set(SPEED, 0);
            }
            while (timer.get_time() < 2) {
                set(SPEED, 4);
            }
            set(SPEED, 0);
            printf ("Path Task Done!!\n");
            return TASK_DONE;

        }

        // Ensure debug messages are printed
        fflush(stdout);
        // Exit if instructed to
        char c = cvWaitKey(TASK_WK);
        if (c != -1) {
            CharacterStreamSingleton::get_instance().write_char(c);
        }
        if (CharacterStreamSingleton::get_instance().wait_key(1) == 'q'){
            stop();
            ret_code = TASK_QUIT;
            break;
        }
    }

    if(done_path){
        ret_code = TASK_DONE;
    }

    return ret_code;
}



MDA_TASK_PATH_SKIP::MDA_TASK_PATH_SKIP (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_PATH_SKIP::~MDA_TASK_PATH_SKIP ()
{
}


MDA_TASK_RETURN_CODE MDA_TASK_PATH_SKIP::run_task() {
    puts("Press q to quit");

    MDA_VISION_MODULE_PATH path_vision;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;

    bool done_skip = false;

    TIMER t;

    // sink to starting depth
    //set (DEPTH, MDA_TASK_BASE::starting_depth+PATH_DELTA_DEPTH);
    set (DEPTH, 500);
    set (DEPTH, 600);
    set (DEPTH, 750);

    t.restart();
    while (t.get_time() < 6) {
        set (SPEED, 10);
    }
    set (SPEED, 0);
/*
    while (1) {
        IplImage* frame = image_input->get_image(DWN_IMG);
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }
        MDA_VISION_RETURN_CODE vision_code = path_vision.filter(frame);

        // clear fwd image
        image_input->ready_image(FWD_IMG);

        if (vision_code == FULL_DETECT) {
            // Get path out of vision
            move(FORWARD, 1);
            // Reset back to 0
            images_checked = 0;
        } else {
            images_checked++;
            if (images_checked >= num_images_to_check) {
                done_skip = true;
                break;
            }
        }

        // Ensure debug messages are printed
        fflush(stdout);
        // Exit if instructed to
        char c = cvWaitKey(TASK_WK);
        if (c != -1) {
            CharacterStreamSingleton::get_instance().write_char(c);
        }
        if (CharacterStreamSingleton::get_instance().wait_key(1) == 'q'){
            ret_code = TASK_QUIT;
            break;
        }
    }
*/
    stop();

    if(done_skip){
        ret_code = TASK_DONE;
    }

    return ret_code;
}
