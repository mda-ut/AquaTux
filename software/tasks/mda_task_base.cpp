#include <cv.h>
#include <highgui.h>
#include <assert.h>

#include "mda_tasks.h"

#define DEPTH_INTERVAL 50

// Default starting depth (target to surface to)
int MDA_TASK_BASE::starting_depth = 275;

void MDA_TASK_BASE::move(ATTITUDE_CHANGE_DIRECTION direction, int delta_accel)
{
  const int DEFAULT_SPEED = 1;

  ATTITUDE_DIRECTION dir;
  switch (direction) {
    case LEFT:
    case RIGHT:
      dir = YAW;
      printf("Turning %s %d degrees\n", (direction == LEFT && delta_accel > 0) || (direction == RIGHT && delta_accel < 0) ? "left" : "right", abs(delta_accel));
      break;
    case RISE:
    case SINK:
      dir = DEPTH;
      printf("Moving %s %d cm\n", (direction == RISE && delta_accel > 0) || (direction == SINK && delta_accel < 0) ? "up" : "down", abs(delta_accel));
      break;
    default:
      dir = SPEED;
      assert(delta_accel > 0);
      printf("Moving %s for %d seconds\n", (direction == FORWARD) ? "forward" : "in reverse", delta_accel);
      break;
  }

  fflush(stdout);

  // Forward or reverse unit is in seconds (use a timer)
  if (dir == SPEED) {
    actuator_output->set_attitude_change(direction, DEFAULT_SPEED);
    time_t start_time = time(NULL);
    while (1) {
      image_input->ready_image(FWD_IMG);
      image_input->ready_image(DWN_IMG);

      char c = cvWaitKey(TASK_WK);
      if (c != -1) {
        CharacterStreamSingleton::get_instance().write_char(c);
      }
      if (c == 'q') {
        return;
      }

      if (difftime(time(NULL), start_time) > (double)delta_accel) {
        break;
      }
    }
  } else if (dir == DEPTH) {
    if (delta_accel < 0) {
      direction = (direction == SINK) ? RISE : SINK;
      delta_accel *= -1;
    }
    // Change the depth by increments
    while (delta_accel != 0) {
      actuator_output->set_attitude_change(direction, std::min(DEPTH_INTERVAL, delta_accel));
      delta_accel -= std::min(DEPTH_INTERVAL, delta_accel);
      stabilize(dir);
    }
  } else {
    // Send the command
    actuator_output->set_attitude_change(direction, delta_accel);  
    stabilize(dir);
  }
}

void MDA_TASK_BASE::set(ATTITUDE_DIRECTION dir, int val)
{
  // if direction is DEPTH, call move() to go to that depth without returning until we reach the depth
  if (dir == DEPTH) {
    move(SINK, val - attitude_input->depth());
  } else {
  // otherwise just set the target yaw
    actuator_output->set_attitude_absolute(dir, val);  
    stabilize(dir);
  }
}

void MDA_TASK_BASE::stop()
{
  actuator_output->stop();
}

void MDA_TASK_BASE::stabilize(ATTITUDE_DIRECTION dir)
{
  if (dir == SPEED) {
    return; // No need to stabilize
  }

  const int yaw_threshold = 3, depth_threshold = 15;
  double max_elapsed_seconds = 10.;
  time_t start_time = time(NULL);
  while (1) {
    image_input->ready_image(FWD_IMG);
    image_input->ready_image(DWN_IMG);

    char c = cvWaitKey(TASK_WK);
    if (c != -1) {
      CharacterStreamSingleton::get_instance().write_char(c);
    }
    if (c == 'q') {
      return;
    }

    // Exit in max_elapsed_seconds to prevent hanging
    if (difftime(time(NULL), start_time) > max_elapsed_seconds) {
      break;
    }

    if (dir == YAW) {
      int current_yaw = attitude_input->yaw();
      int target_yaw = actuator_output->get_target_attitude(YAW);
      if (abs(current_yaw - target_yaw) <= yaw_threshold ||
          abs(current_yaw - target_yaw) >= 360 - yaw_threshold) {
        break;
      }
    } else if (dir == DEPTH) {
      int current_depth = attitude_input->depth();
      int target_depth = actuator_output->get_target_attitude(DEPTH);
      if (abs(current_depth - target_depth) <= depth_threshold) {
        break;
      }
    }
  }
}
