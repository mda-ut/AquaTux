#include "mda_tasks.h"
#include "mda_vision.h"

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
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;
    int GATE_DEPTH;
    GATE_DEPTH = attitude_input->depth() + 150;
    set (DEPTH, GATE_DEPTH);
    int starting_yaw = attitude_input->yaw();
    printf("Starting yaw: %d\n", starting_yaw);
    set (YAW, starting_yaw);
    int counter = 0;
    while (1)
    {
        set (SPEED, 5);
        if (counter % 50 == 0)
        {
	    set (DEPTH, GATE_DEPTH);
	    printf("Adam: set depth to %d\n", GATE_DEPTH);
            set (YAW, starting_yaw);
            printf("Rose: set yaw = %d\n", starting_yaw);
            counter = 0;
        }
        counter ++;
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
