#include "mda_vision.h"
#include "mda_tasks.h"

const float LEN_TO_WIDTH_MAX = 9.0;
const float LEN_TO_WIDTH_MIN = 4.0;

MDA_TASK_BUOY:: MDA_TASK_BUOY (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_BUOY:: ~MDA_TASK_BUOY ()
{
}

enum TASK_STATE {
    STARTING, 
    STOPPED,    // stopped to check for if there are buoys
    PANNING,    // panning around
    FIND_COLOR, // buoy found, checking buoy color and cycling status
    APPROACH    // buoy found, approaching buoy
};

MDA_TASK_RETURN_CODE MDA_TASK_BUOY:: run_task() {
    static int buoy_index = 0;
    MDA_TASK_RETURN_CODE code = run_single_buoy(buoy_index, BUOY_RED);
    if (code == TASK_DONE) {
        buoy_index = 1;
    }
    return TASK_DONE;
}

void get_data_from_frame (MDA_VISION_MODULE_BUOY* vision, bool* valid, int* ang_x, int* ang_y, int* range, int* color, int index=-1) {
    // get the buoy parameters if available
    for (int i = 0; i <= 1; i++) {
        if (index < 0 || i == index) {
            valid[i] = vision->rbox_stable(i, 30);
            ang_x[i] = vision->get_angular_x();
            ang_y[i] = vision->get_angular_y();
            range[i] = vision->get_range();
            color[i] = vision->get_color();
        }
        else {
            valid[i] = false;
        }
    }
}

MDA_TASK_RETURN_CODE MDA_TASK_BUOY:: run_single_buoy(int buoy_index, BUOY_COLOR color) {
    puts("Press q to quit");

    assert (buoy_index >= 0 && buoy_index <= 1);
    
    MDA_VISION_MODULE_BUOY buoy_vision;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;

    /// Here we store the starting attitude vector, so we can return to this attitude later
    int starting_yaw = attitude_input->yaw();
    printf("Starting yaw: %d\n", starting_yaw);

    //set (DEPTH, 400);

    TASK_STATE state = STARTING;
    bool done_buoy = false;
    static TIMER timer;
    static TIMER master_timer;
    timer.restart();
    master_timer.restart();


//###### hack code for competition
    set (SPEED, 0);
    
    int hack_depth, hack_time;
    read_mv_setting ("hacks.csv", "BUOY_DEPTH", hack_depth);
    read_mv_setting ("hacks.csv", "BUOY_TIME", hack_time);
    printf ("Buoy: going to depth %d\n", hack_depth);
        fflush(stdout);
    if (hack_depth > 500)
	set (DEPTH, 500);
    if (hack_depth > 600)
        set (DEPTH, 600);
    set (DEPTH, hack_depth);
    set (YAW, starting_yaw);

    printf ("Buoy: moving forward for %d seconds\n", hack_time);
        fflush(stdout);
    timer.restart();
    while (timer.get_time() < hack_time) {
        set (SPEED, 8);
    }
    set(SPEED, 0);

    if (hack_depth > 600)
         set (DEPTH, 600);
    if (hack_depth > 500)
        set (DEPTH, 500);
    set (YAW, starting_yaw);
    return TASK_DONE;
//###### end hack code for competition
    /**
    * Basic Algorithm
    *  - Assume for now that we just want to hit the cylindrical buoys when they're red
    *  - We want to search for 1 or 2 buoys. If we find both:
    *    - Are both non-red and/or cycling? Go for the one indicated by buoy_index
    *    - Is one non-red and/or cycling? Go for that one
    *    - Do we only see one buoy? Go there if non-red and/or cycling
    *
    */
    
    while (1) {
        IplImage* frame = image_input->get_image();
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }

        MDA_VISION_RETURN_CODE vision_code = buoy_vision.filter(frame);
        (void) vision_code;
        // clear dwn image - RZ: do we need this?
        //int down_frame_ready = image_input->ready_image(DWN_IMG);
        //(void) down_frame_ready;

        bool valid[2] = {false, false};
        int ang_x[2], ang_y[2];
        int range[2];
        int color[2];
        static bool color_cycling[2] = {false, false};
        static int curr_index = -1;
        static int prev_time = -1;
        static int prev_color = -1;
        static int n_valid_color_find_frames = 0;

        if (!done_buoy) {
            // state machine
            if (state == STARTING) {
                // here we just move forward until we find something, and pan if we havent found anything for a while
                printf ("Starting: Moving Foward for 1 meter\n");
                move (FORWARD, 1);

                if (timer.get_time() > 1) {
                    set (SPEED, 0);
                    timer.restart();
                    buoy_vision.clear_frames();
                    state = STOPPED;
                }
            }
            else if (state == STOPPED) {
                if (timer.get_time() < 0) {
                    printf ("Stopped: Collecting Frames\n");
                }
                else {
                    get_data_from_frame (&buoy_vision, valid, ang_x, ang_y, range, color);

                    if (!valid[0] && !valid[1]) { // no buoys
                        printf ("Stopped: No target\n");
                        if (master_timer.get_time() > 60) { // we've seen nothing for 60 seconds
                            printf ("Master Timer Timeout!!\n");
                            return TASK_MISSING;
                        }
                        if (timer.get_time() > 2) {
                            printf ("Stopped: Timeout\n");
                            timer.restart();
                            state = STARTING;
                        }
                    }
                    else { // turn towards the buoy we want
                        // chose buoy
                        if (valid[0] && valid[1])
                            curr_index = buoy_index;
                        else
                            curr_index = valid[0] ? 0 : 1;

                        printf ("Stopped: Identified buoy %d (%s) as target\n", curr_index, color_int_to_string(color[curr_index]).c_str());
                        move (RIGHT, ang_x[curr_index]);
                        
                        timer.restart();
                        master_timer.restart();
                        prev_color = color[curr_index];
                        prev_time = -1;
                        buoy_vision.clear_frames();
                        n_valid_color_find_frames = 0;
                        state = FIND_COLOR;
                    }
                }
            }
            else if (state == FIND_COLOR) {
                // stare for 6 seconds, check if the buoy color changes
                int t = timer.get_time();
                if (t >= 4) {
                    if (n_valid_color_find_frames <= 2) {
                        printf ("Find Color: Not enough good frames (%d).\n", n_valid_color_find_frames);
                        timer.restart();
                        master_timer.restart();
                        state = STARTING;
                    }
                    else if (color_cycling[curr_index] || prev_color != MV_RED) {
                        printf ("Find Color: Finished. Must approach buoy %d.\n", curr_index);
                        timer.restart();
                        master_timer.restart();
                        state = APPROACH;
                    }
                    else {
                        printf ("Find Color: Finished. No need to do buoy %d.\n", curr_index);
                        done_buoy = true;
                        return TASK_QUIT;
                    }
                }
                else if (t != prev_time) {
                    printf ("Find_Color: examined buoy %d for %d seconds.\n", curr_index, t);
                    get_data_from_frame (&buoy_vision, valid, ang_x, ang_y, range, color, curr_index);

                    if (valid[curr_index]) {  
                        if (color[curr_index] != prev_color) {
                            printf ("\tFound buoy %d as color cycling.\n", curr_index);
                            color_cycling[curr_index] = true;
                        }
                        prev_time = t;
                        n_valid_color_find_frames++;
                    } 
                }
            }
            else if (state == APPROACH) {
                get_data_from_frame (&buoy_vision, valid, ang_x, ang_y, range, color, curr_index);
                if (valid[curr_index]) {
                    printf ("Approach[%d]: range=%d, ang_x=%d\n", curr_index, range[curr_index], ang_x[curr_index]);
                    if (range[curr_index] > 100) { // long range = more freedom to turn/sink
                        if (abs(ang_x[curr_index]) > 5) {
                            set(SPEED, 0);
                            move(RIGHT, ang_x[curr_index]);
                            buoy_vision.clear_frames();
                        }    
                        /*else if (tan(ang_y[curr_index])*range[curr_index] > 50) { // depth in centimeters
                            set(SPEED, 0);
                            move (SINK, 25); // arbitrary rise for now
                        }*/
                        else {
                            set(SPEED, 1);
                        }
                    }
                    else {
                        if (abs(ang_x[curr_index]) > 10) {
                            set(SPEED, 0);
                            move(RIGHT, ang_x[curr_index]);
                            buoy_vision.clear_frames();
                        }
                        else {
                            done_buoy = true;
                        }   
                    }

                    timer.restart();
                    master_timer.restart();
                }
                else {
                    set(SPEED, 0);
                    if (timer.get_time() > 4) { // 4 secs without valid input
                        printf ("Approach: Timeout");
                        timer.restart();
                        state = STOPPED;
                    }
                }
            }
        } // done_buoy
        else { // done_buoy
            // charge forwards, then retreat back some number of meters, then realign sub to starting attitude
            printf("Ramming buoy\n");
            timer.restart();
            while (timer.get_time() < 2)
                move(FORWARD, 2);
            stop();

            // retreat backwards
            printf("Reseting Position\n");
            timer.restart();
            while (timer.get_time() < 3)
                move(REVERSE, 2);
            stop();

            ret_code = TASK_DONE;
            break;
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

    return ret_code;
}
