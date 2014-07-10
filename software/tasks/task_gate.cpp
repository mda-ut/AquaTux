#include "mda_tasks.h"
#include "mda_vision.h"

const int PAN_TIME_HALF = 6;
const int MASTER_TIMEOUT = 200;
const int GATE_DELTA_DEPTH = 50;

MDA_TASK_GATE::MDA_TASK_GATE (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_GATE::~MDA_TASK_GATE ()
{
}

enum TASK_STATE {
    STARTING,
    SLOW_FOWARD,
    STOPPED,
    PANNING,
    APPROACH
};

MDA_TASK_RETURN_CODE MDA_TASK_GATE::run_task() {
    puts("Press q to quit");

    MDA_VISION_MODULE_GATE gate_vision;
    TASK_STATE state = STARTING;

    bool done_gate = false;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;

    // read the starting orientation
    int starting_yaw = attitude_input->yaw();
    printf("Starting yaw: %d\n", starting_yaw);
    
    MDA_TASK_BASE::starting_depth = attitude_input->depth();
    // gate depth
    set (DEPTH, HARDCODED_DEPTH-50/*MDA_TASK_BASE::starting_depth+GATE_DELTA_DEPTH*/);
    set (DEPTH, HARDCODED_DEPTH/*MDA_TASK_BASE::starting_depth+GATE_DELTA_DEPTH*/);

    // go to the starting orientation in case sinking changed it
    set (YAW, starting_yaw);

    static TIMER timer; // keeps track of time spent in each state
    static TIMER master_timer; // keeps track of time spent not having found the target
    static TIMER full_detect_timer; // keeps track of time since the last full detect
    timer.restart();
    master_timer.restart();
    full_detect_timer.restart();

    while (1) {
        IplImage* frame = image_input->get_image();
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }
        MDA_VISION_RETURN_CODE vision_code = gate_vision.filter(frame);

        // clear dwn image. RZ - do we need this?
        // This ensures the other camera is properly logged
        // and that the webcam cache is cleared so it stays in sync - VZ
        image_input->ready_image(DWN_IMG);

        // static
        //static int prev_t = -1;

        /**
        * Basic Algorithm: (repeat)
        *  - Go straight foward in STARTING state until we see anything
        *  
        *    - If we saw a ONE_SEGMENT, calculate the course we should take to allow the segment to remain in view.
        *    - If we saw a FULL_DETECT, change course to face it
        *  - Always go forwards in increments and stop for 1 seconds to stare each time.
        */
        if (!done_gate) {
            if (state == STARTING) {
                printf ("Starting: Moving Foward at High Speed\n");
                set (SPEED, 9);

                if (master_timer.get_time() > MASTER_TIMEOUT) {
                    printf ("Master Timer Timeout!!\n");
                    return TASK_MISSING;
                }
                else if (vision_code == FULL_DETECT) {
                    printf ("FAST Foward: Full Detect\n");
                    //int ang_x = gate_vision.get_angular_x();
                    //set_yaw_change(ang_x);

                    if (gate_vision.get_range() < 420) {
                        done_gate = true;
                        printf ("Range = %d, Approaching Gate\n", gate_vision.get_range());
                    }

                    timer.restart();
                    full_detect_timer.restart();
                    master_timer.restart();
                }
                /*if (gate_vision.latest_frame_is_valid()) {
                    set (SPEED, 0);
                    master_timer.restart();
                    timer.restart();
                    gate_vision.clear_frames();
                    state = SLOW_FOWARD;
                }*/
            }
            else if (state == SLOW_FOWARD) {
                printf ("Slow Foward: Moving foward a little\n");
                set (SPEED, 4);

                if (timer.get_time() > 3) {
                    timer.restart();
                    gate_vision.clear_frames();
                    state = PANNING;
                }
                else if (vision_code == FULL_DETECT) {
                    printf ("Slow Foward: Full Detect\n");
                    int ang_x = gate_vision.get_angular_x();
                    set_yaw_change(ang_x);

                    if (gate_vision.get_range() < 420) {
                        done_gate = true;
                        printf ("Range = %d, Approaching Gate\n", gate_vision.get_range());
                    }

                    timer.restart();
                    full_detect_timer.restart();
                    master_timer.restart();
                }
                if (master_timer.get_time() > MASTER_TIMEOUT) {
                    printf ("Master Timer Timeout!!\n");
                    return TASK_MISSING;
                }
            }
            else if (state == STOPPED) {
                // if havent spent 1 second in this state, keep staring
                /*if (timer.get_time() < 1) {
                    printf ("Stopped: Collecting Frames\n");
                }
                else {*/
                    if (vision_code == NO_TARGET) {
                        printf ("Stopped: No target\n");
                        if (master_timer.get_time() > 60) { // we've seen nothing for 60 seconds
                            printf ("Master Timer Timeout!!\n");
                            return TASK_MISSING;
                        }
                        if (timer.get_time() > 3) {
                            printf ("Stopped: Timeout\n");
                            timer.restart();
                            state = SLOW_FOWARD;
                        }
                    }
                    else if (vision_code == ONE_SEGMENT) {
                        printf ("Stopped: One Segment\n");
                        //int ang_x = gate_vision.get_angular_x();

                        // if segment too close just finish
                        if (gate_vision.get_range() < 240) {
                            printf ("One Segment with range too low. Ending task.\n");
                            done_gate = true;
                        }
                        else if (gate_vision.get_range() < 470 && full_detect_timer.get_time() > 10) {
                            timer.restart();
                            master_timer.restart();
                            //prev_t = -1;
                            //state = PANNING;
                        }
                        // only execute turn if the segment is close to out of view and no other options
                        /*else if (ang_x >= 40) {
                            ang_x -= 20;
                            printf ("Moving Left on One Segment %d Degrees\n", ang_x);
                            move (RIGHT, ang_x);
                            gate_vision.clear_frames();
                        }
                        else if (ang_x <= -40) {
                            ang_x += 20;
                            printf ("Moving Left on One Segment %d Degrees\n", ang_x);
                            move (RIGHT, ang_x);
                            gate_vision.clear_frames();
                        }*/

                        if (timer.get_time() > 2) {
                            timer.restart();
                            master_timer.restart();
                            state = SLOW_FOWARD;
                        }
                    }
                    else if (vision_code == FULL_DETECT) {
                        printf ("Stopped: Full Detect\n");
                        int ang_x = gate_vision.get_angular_x();
                        move (RIGHT, ang_x);

                        if (gate_vision.get_range() < 420) {
                            done_gate = true;
                            printf ("Range = %d, Approaching Gate\n", gate_vision.get_range());
                        }

                        timer.restart();
                        full_detect_timer.restart();
                        master_timer.restart();
                        state = SLOW_FOWARD;
                    }
                    else {
                        printf ("Error: %s: line %d\ntask module recieved an unhandled vision code.\n", __FILE__, __LINE__);
                        exit(1);
                    }
                //} // collecting frames
            } // state
            else if (state == PANNING) { // pan and look for a frame with 2 segments
                /*printf ("Panning\n");
                int t = timer.get_time();
                if (t < PAN_TIME_HALF && t != prev_t) { // pan left for first 6 secs
                    set_yaw_change (-20);
                    prev_t = timer.get_time();
                }
                else if (t < 3*PAN_TIME_HALF && t != prev_t) { // pan right for next 10 secs
                    set_yaw_change (20);
                    prev_t = timer.get_time();
                }
                else if (t >= 3*PAN_TIME_HALF) { // stop pan and reset
                    printf ("Pan completed\n");
                    set (YAW, starting_yaw);
                    gate_vision.clear_frames();
                    master_timer.restart();
                    timer.restart();
                    state = STOPPED;
                }
                else if (gate_vision.latest_frame_is_two_segment()) {
                    printf ("Two segment frame found - stopping pan\n");
                    set_yaw_change (0);
                    gate_vision.clear_frames();
                    master_timer.restart();
                    full_detect_timer.restart();
                    timer.restart();
                    state = STOPPED;
                }*/
            }
            else if (state == APPROACH) {
            }
        } // done_gate
        else {
            // charge foward for 2 secs
            //set(YAW, starting_yaw);
            timer.restart();
            while (timer.get_time() < 2) {
                set(SPEED, 6);
            }
            set(SPEED, 0);
            printf ("Gate Task Done!!\n");
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

    return ret_code;
}
