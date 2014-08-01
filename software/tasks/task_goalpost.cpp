#include "mda_vision.h"
#include "mda_tasks.h"

const int MASTER_TIMEOUT = 240;

MDA_TASK_GOALPOST::MDA_TASK_GOALPOST (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_GOALPOST::~MDA_TASK_GOALPOST ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_GOALPOST::run_task() {
    puts("Press q to quit");


    MDA_VISION_MODULE_GOALPOST goalpost_vision;
    MDA_TASK_RETURN_CODE ret_code = TASK_MISSING;
    
    int starting_yaw = attitude_input->yaw();
    printf("Starting yaw: %d\n", starting_yaw);

    bool done_goalpost = false;

    int GOALPOST_DEPTH = attitude_input->depth()+250;
    /*read_mv_setting ("hacks.csv", "GOALPOST_DEPTH", GOALPOST_DEPTH);
    
    if (GOALPOST_DEPTH > 500)
        set(DEPTH, 500);
    if (GOALPOST_DEPTH > 600)
        set(DEPTH, 600);*/
    set(DEPTH, GOALPOST_DEPTH);
    set(YAW, starting_yaw);

    //hacks for semi
    TIMER t;
    t.restart();
    while (t.get_time() < 25)
    {
        set(SPEED, 5);
    }
    stop();
    set(YAW, attitude_input->yaw() - 30);
    t.restart();
        while (t.get_time() < 25)
    {
        set(SPEED, 5);
    }
    stop();

    TIMER timer;
    timer.restart();

    while (1) {
        IplImage* frame = image_input->get_image();
        int counter = 0;
        if (!frame) {
            ret_code = TASK_ERROR;
            break;
        }
        MDA_VISION_RETURN_CODE vision_code = goalpost_vision.filter(frame);
        printf("Rose: Goalpost vision_code: %d\n", vision_code);
        
        // clear dwn image
        int down_frame_ready = image_input->ready_image(DWN_IMG);
        (void) down_frame_ready;

        if(!done_goalpost){
            if (vision_code == FATAL_ERROR) {
                ret_code = TASK_ERROR;
                break;
            }
            else if (vision_code == NO_TARGET) {
                counter ++;
                t.restart();
                if (counter % 8 == 0)
                    while(t.get_time()<3)
                    {
                        set(SPEED, 5);
                    }
                if (counter % 10 == 0)
                    set(YAW, attitude_input->yaw()-15);
                else if (counter % 5 == 0)
                    set(YAW, attitude_input->yaw()+ 15);
                stop();
                printf("Rose: Goalpost not found! looking around\n");
                if (timer.get_time() > MASTER_TIMEOUT) {
                    stop();
                    return TASK_MISSING;
                }
            }
            else if (vision_code == FULL_DETECT || vision_code == ONE_SEGMENT) {
                int ang_x = goalpost_vision.get_angular_x();
                int ang_y = goalpost_vision.get_angular_y();
                int range = goalpost_vision.get_range();
                int depth_change = tan(ang_y*0.017453) * range; 

                // if we can see full goalpost and range is less than 200 we are done the frame part
                if (goalpost_vision.get_range() < 300) {
                    timer.restart();
                    while (timer.get_time() < 7) { //charging forward after task is done
                        set (SPEED, 5);
                    }

                    int counter = 0;
                    while (counter < 5)
                    {
                        stop();
                        
                        printf("Rose: changing yaw: %d\n", attitude_input->yaw());
                        
                        set(YAW, attitude_input->yaw()-100);
                        timer.restart();
                        while (timer.get_time() < 2) { //charging forward after task is done
                            set (SPEED, 5);
                        }
                        counter ++;
                    }

                    stop();
                    done_goalpost = true;
                    ret_code = TASK_DONE;
                    break;
                }

                if(fabs(ang_y) > 10.0 && range < 800) {
                    stop();
                    move(SINK, depth_change/2);// /2 for hacks
                    printf("Goalpost: Move: adjusting depth\n");
                }
                else if(abs(ang_x) > 10.0) {
                    stop();
                    move(RIGHT, ang_x);
                    printf("Goalpost: Move: adjusting yaw\n");
                }
                else {
                    set(SPEED, 5);
                }
            }
            else {
                printf ("Error: %s: line %d\ntask module recieved an unhandled vision code.\n", __FILE__, __LINE__);
                exit(1);
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
            stop();
            ret_code = TASK_QUIT;
            break;
        }
    }

    return ret_code;
}
