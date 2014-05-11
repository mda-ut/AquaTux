#include "mda_tasks.h"
#include "mda_vision.h"

MDA_TASK_TEST:: MDA_TASK_TEST (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o),
    window ("task_test")
{
}

MDA_TASK_TEST:: ~MDA_TASK_TEST ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_TEST:: run_task() {
    puts("Press q to quit");

    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;

    enum state {SINK_STATE, FWD_STATE, RISE_STATE, REV_STATE};

    MDA_VISION_MODULE_BUOY vision_buoy;
    TIMER t;

    while (1) {
        IplImage* frame = image_input->get_image(FWD_IMG);
        if (!frame) {
            break;
        }
        window.showImage (frame);

        image_input->ready_image(DWN_IMG);

        vision_buoy.filter(frame);
        if (t.get_time() > 6) {
            t.restart();
            vision_buoy.clear_frames();
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
