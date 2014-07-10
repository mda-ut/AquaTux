#include "mda_tasks.h"

MDA_TASK_SURFACE::MDA_TASK_SURFACE (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_SURFACE::~MDA_TASK_SURFACE ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_SURFACE::run_task() {
    puts("Surfacing");
    fflush(stdout);

    TIMER t;
    if (attitude_input->depth() > 500) {
      set(DEPTH, 500);
    }
    if (attitude_input->depth() > 400) {
      set(DEPTH, 400);
    }
    if (attitude_input->depth() > 270) {
      set(DEPTH, MDA_TASK_BASE::starting_depth);
    }

    return TASK_DONE;
}


MDA_TASK_RESET::MDA_TASK_RESET (AttitudeInput* a, ImageInput* i, ActuatorOutput* o) :
    MDA_TASK_BASE (a, i, o)
{
}

MDA_TASK_RESET::~MDA_TASK_RESET ()
{
}

MDA_TASK_RETURN_CODE MDA_TASK_RESET::run_task(int depth_in_mv, int yaw, int fwd_movement_time) {
    char buffer[200];
    sprintf (buffer, "Task Reset. Resetting to depth=%d and moving forward for %d seconds.\n", depth_in_mv, fwd_movement_time);
    puts(buffer);
    fflush(stdout);

    int speed = 2;
    if (fwd_movement_time < 0) {
    	fwd_movement_time = abs(fwd_movement_time);
    	speed = -2;
    }

    TIMER timer;

    // go to right depth
    set(DEPTH, depth_in_mv);
    // wait 2 sec
    timer.restart();
    while (timer.get_time() < 1);
    
    // go to right orientation
    actuator_output->set_attitude_absolute(YAW, yaw);
    // wait 2 sec
    timer.restart();
    while (timer.get_time() < 1);

    // move foward
    while (timer.get_time() < fwd_movement_time) {
    	set(SPEED, speed);	
    }
    set(SPEED, 0);
    // wait 1 second
    timer.restart();
    while (timer.get_time() < 1);

    return TASK_DONE;
}
