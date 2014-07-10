#include "mda_tasks.h"
#include "mda_vision.h"

MDA_TASK_MARKER::MDA_TASK_MARKER (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_MARKER::~MDA_TASK_MARKER ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_MARKER::run_task() {
    puts("Press q to quit");

    MDA_VISION_MODULE_MARKER marker_vision;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;
    bool done_marker1 = false;
    bool done_marker2 = false;

    static const int starting_depth = 630;

    // this is rough depth of the marker dropper targets
    set(DEPTH, starting_depth);

    while (true) {
        IplImage* frame = image_input->get_image(DWN_IMG);
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }
        MDA_VISION_RETURN_CODE vision_code = marker_vision.filter(frame);

        // clear fwd image
        int fwd_frame_ready = image_input->ready_image();
        (void) fwd_frame_ready;
        
        if (vision_code == FATAL_ERROR) {
            ret_code = TASK_ERROR;
            break;
        }
        else if (vision_code == NO_TARGET || vision_code == UNKNOWN_TARGET) {
            set(SPEED, 1);
        }
        else if (vision_code == FULL_DETECT) {
            bool *targets_found = marker_vision.getFound();
            bool new_target = false;
            if (targets_found[0] && !done_marker1) {
               done_marker1 = true;
               new_target = true;
            } else if (targets_found[1] && !done_marker2) {
               done_marker2 = true;
               new_target = true;
            }
            if (new_target) {
                printf("Dropping marker here.\n");
                fflush(stdout);
                stop();
                sleep(2);
            }

            if (targets_found[0] && targets_found[1]) {
                ret_code = TASK_DONE;
                printf ("Done marker task.\n");
                break;
            }
        }
        else {
            printf ("Error: %s: line %d\ntask module recieved an unhandled vision code.\n", __FILE__, __LINE__);
            exit(1);
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
