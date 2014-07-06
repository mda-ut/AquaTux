#include <assert.h>
#include <cv.h>

#include "Mission.h"
#include "Operation.h"
#include "CharacterStreamSingleton.h"

#define BUF_LEN 16

// Uncomment to debug videos frame by frame
//#define DEBUG_FRAME_BY_FRAME

void ManualOperation::display_start_message()
{
  // clear regular I/O
  fflush(stdout);

  // ncurses stuff
  initscr();
  clear();
  cbreak();

  // info message
  printw(
         "Commands:\n"
         "  q    - exit simulator\n"
         "  z    - save input image screenshots as image_[fwd/dwn].jpg\n"
         "  y    - toggle display of raw input image stream\n"
         "  u    - switch between fwd and dwn webcam (only in vision mode or if 2 webcams disabled)\n"
         "\n"
         "  wasd - use controller to move forward/reverse/left/right\n"
         "  rf   - use controller to move up/down\n"
         "  e    - stop\n"
         "\n"
         "  v    - enter vision mode\n"
         "  #    - input exact target attitude\n"
         "\n"
         "  1    - run gate task\n"
         "  2    - run path task\n"
         "  3    - run buoy task\n"
         "  4    - run frame task\n"
         "  5    - run marker dropper task\n"
         "  8    - run path skip task\n"
         "  9    - run surface task\n"
         "  0    - run test task\n"
         "  m    - run mission\n"
         "\n"
         "Simulator only commands:\n"
         "  ijkl - move forward/reverse/left/right\n"
         "  p;   - move up/down\n"
         "  ' '  - nullify all speed and acceleration\n"
         "\n"
         "Submarine only commands:\n"
         " ^     - power on submarine\n"
         " %%     - submarine startup sequence\n"
         " $     - power off submarine\n"
         "\n");
  refresh();
}

#define SPEED_CHG 18
#define YAW_CHG_IN_DEG 15
#define DEPTH_CHG_IN_CM 50

#define REFRESH_RATE_IN_HZ 10
#define WAIT_KEY_IN_MS 5

void ManualOperation::work()
{
#ifndef DEBUG_FRAME_BY_FRAME
  // Turn off display by default
  if (image_input->can_display()) {
    mvWindow::setShowImage(show_raw_images);
  }
#else
  // Show first image
  process_image();
#endif

  display_start_message();

  // Take keyboard commands
  bool loop = true;
  while (loop) {
    char c = CharacterStreamSingleton::get_instance().wait_key(WAIT_KEY_IN_MS);

    // Print yaw and depth unless delayed by another message
    if (c == '\0') {
      if (count < 0) {
        count++;
      } else if (mode != VISION) {
        static int attitude_counter = 0;
        attitude_counter++;
        if (attitude_counter == 1000 / WAIT_KEY_IN_MS / REFRESH_RATE_IN_HZ) {
          attitude_counter = 0;
          char buf[128];
          sprintf(buf, "Yaw: %+04d degrees, Depth: %+04d cm, Target Yaw: %+04d degrees, Target Depth: %+04d",
            attitude_input->yaw(), attitude_input->depth(), attitude_input->target_yaw(), attitude_input->target_depth());
          message(buf);
        }
      }
    }

    switch(c) {
      case 'q':
         loop = false;
         break;
      case 'z':
         dump_images();
         break;
      case 'y':
         if (image_input->can_display()) {
           show_raw_images = !show_raw_images;
           mvWindow::setShowImage(show_raw_images);
         } else {
           message_hold("Image stream should already be displayed");
         }
         break;
      case 'u':
         use_fwd_img = !use_fwd_img;
         break;
      case 'i':
         actuator_output->special_cmd(SIM_MOVE_FWD);
         break;
      case 'k':
         actuator_output->special_cmd(SIM_MOVE_REV);
         break;
      case 'j':
         actuator_output->special_cmd(SIM_MOVE_LEFT);
         break;
      case 'l':
         actuator_output->special_cmd(SIM_MOVE_RIGHT);
         break;
      case 'p':
         actuator_output->special_cmd(SIM_MOVE_RISE);
         break;
      case ';':
         actuator_output->special_cmd(SIM_MOVE_SINK);
         break;
      case 'e':
         actuator_output->stop();
         break;
      case 'w':
         actuator_output->set_attitude_change(FORWARD, SPEED_CHG);
         break;
      case 's':
         actuator_output->set_attitude_change(REVERSE, SPEED_CHG);
         break;
      case 'a':
         actuator_output->set_attitude_change(LEFT, YAW_CHG_IN_DEG);
         break;
      case 'd':
         actuator_output->set_attitude_change(RIGHT, YAW_CHG_IN_DEG);
         break;
      case 'r':
         actuator_output->set_attitude_change(RISE, DEPTH_CHG_IN_CM);
         break;
      case 'f':
         actuator_output->set_attitude_change(SINK, DEPTH_CHG_IN_CM);
         break;
      case ' ':
         actuator_output->special_cmd(SIM_ACCEL_ZERO);
#ifdef DEBUG_FRAME_BY_FRAME
         process_image();
#endif
         break;
      case '^':
         actuator_output->special_cmd(SUB_POWER_ON);
         break;
      case '%':
         actuator_output->stop();
         actuator_output->special_cmd(SUB_STARTUP_SEQUENCE);
         break;
      case '$':
         actuator_output->special_cmd(SUB_POWER_OFF);
         break;
      case '#':
         long_input();
         break;
      case 'm':
         endwin();
         // Scope mission so that it is destructed before display_start_message
         {
           Mission m(attitude_input, image_input, actuator_output);
           m.work_internal(true);
         }
         display_start_message();
         message_hold("Mission complete!");
         break;
      case 'M': // same as mission, but force turn off show_image
         endwin();
         // Scope mission so that it is destructed before display_start_message
         {
           Mission m(attitude_input, image_input, actuator_output);
           m.work();
         }
         display_start_message();
         message_hold("Mission complete!");
         break;

      case '0':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_TEST test_task(attitude_input, image_input, actuator_output);
             ret_code = test_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Test task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Test task quit by user");
                break;
             default:
                message_hold("Test task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected test vision module\n");
         vision_module = new MDA_VISION_MODULE_TEST();
         use_fwd_img = true;
         break;
      case '1':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_GATE gate_task(attitude_input, image_input, actuator_output);
             ret_code = gate_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Gate task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Gate task quit by user");
                break;
             default:
                message_hold("Gate task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected gate vision module\n");
         vision_module = new MDA_VISION_MODULE_GATE();
         use_fwd_img = true;
         break;
      case '2':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_PATH path_task(attitude_input, image_input, actuator_output);
             ret_code = path_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Path task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Path task quit by user");
                break;
             default:
                message_hold("Path task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected path vision module\n");
         vision_module = new MDA_VISION_MODULE_PATH();
         use_fwd_img = false;
         break;
      case '3':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_BUOY buoy_task(attitude_input, image_input, actuator_output);
             ret_code = buoy_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Buoy task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Buoy task quit by user");
                break;
             default:
                message_hold("Buoy task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected buoy vision module\n");
         vision_module = new MDA_VISION_MODULE_BUOY();
         use_fwd_img = true;
         break;
      case '4':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_GOALPOST goalpost_task(attitude_input, image_input, actuator_output);
             ret_code = goalpost_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Frame task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Frame task quit by user");
                break;
             default:
                message_hold("Frame task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected frame vision module\n");
         vision_module = new MDA_VISION_MODULE_GOALPOST();
         use_fwd_img = true;
         break;
      case '5':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_MARKER marker_task(attitude_input, image_input, actuator_output);
             ret_code = marker_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Marker task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Marker task quit by user");
                break;
             default:
                message_hold("Marker task errored out");
                break;
           }
           break;
         }
         delete vision_module;
         message_hold("Selected marker dropper vision module\n");
         vision_module = new MDA_VISION_MODULE_MARKER();
         use_fwd_img = false;
         break;
      case '8':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_PATH_SKIP path_skip(attitude_input, image_input, actuator_output);
             ret_code = path_skip.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Path skip task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Path skip task quit by user");
                break;
             default:
                message_hold("Path skip task errored out");
                break;
           }
         }
         break;
      case '9':
         if (mode != VISION) {
           endwin();

           MDA_TASK_RETURN_CODE ret_code;
           // Scope task so that it is destructed before display_start_message
           {
             MDA_TASK_SURFACE surface_task(attitude_input, image_input, actuator_output);
             ret_code = surface_task.run_task();
           }

           display_start_message();

           switch(ret_code) {
             case TASK_DONE:
                message_hold("Surface task completed successfully");
                break;
             case TASK_QUIT:
                message_hold("Surface task quit by user");
                break;
             default:
                message_hold("Surface task errored out");
                break;
           }
         }
         break;
      case 'x':
         if (mode == NORMAL) {
           break;
         }
         delete vision_module;

         vision_module = NULL;
         mode = NORMAL;
         display_start_message();
         break;
      case 'v':
         if (mode == VISION) {
           delete vision_module;
           vision_module = NULL;
         }
         endwin();
         mode = VISION;
         message(
           "Entering Vision Mode:\n"
           "  0    - test vision\n"
           "  1    - gate vision\n"
           "  2    - path vision\n"
           "  3    - buoy vision\n"
           "  4    - frame vision\n"
           "  5    - marker dropper vision\n"
           "\n"
           "  v    - cancel current vision selection\n"
           "  x    - exit vision mode\n"
           "  q    - exit simulator\n"
         );
         break;
      case '\0': // timeout
#ifndef DEBUG_FRAME_BY_FRAME
        process_image();
#else
        char ch = cvWaitKey(3);
        if (ch != -1) {
          CharacterStreamSingleton::get_instance().write_char(ch);
        }
#endif
        break;
    }
  }

  // close ncurses
  endwin();

  // Surface
  MDA_TASK_SURFACE surface(attitude_input, image_input, actuator_output);
  surface.run_task();

  actuator_output->special_cmd(SUB_POWER_OFF);
}

void ManualOperation::process_image()
{
  if (vision_module) {
    IplImage* frame = image_input->get_image(use_fwd_img?FWD_IMG:DWN_IMG);
    if (frame) {
      vision_module->filter(frame);
      // Allow several keys (ie if held) to be read before doing the vision processing loop
      for (int i = 0; i < 3; i++) {
        char ch = cvWaitKey(3);
        CharacterStreamSingleton::get_instance().write_char(ch);
      }
      fflush(stdout);
    } else {
      message_hold("Image stream over");
    }
#ifndef DISABLE_DOUBLE_WEBCAM
    // show the other image by getting it
    image_input->get_image(use_fwd_img?DWN_IMG:FWD_IMG);
#endif
  } else {
    // needs to be called periodically for highgui event-processing
#ifndef DISABLE_DOUBLE_WEBCAM
    image_input->get_image(FWD_IMG);
    image_input->get_image(DWN_IMG);
#else
    image_input->get_image(use_fwd_img?FWD_IMG:DWN_IMG);
#endif
    char ch = cvWaitKey(3);
    if (ch) {
      CharacterStreamSingleton::get_instance().write_char(ch);
    }
  }
}

void ManualOperation::message(const char *msg)
{
  if (mode == VISION) {
    if (strlen(msg) == 0) {
      return;
    }
    printf("%s\n", msg);
    fflush(stdout);
    return;
  }

  int x, y;
  getmaxyx(stdscr, y, x);
  assert(y > 0);

  putchar('\r');
  for (int i = 0; i < x; i++) {
    putchar(' ');
  }
  printf("\r%s", msg);

  fflush(stdout);
}

void ManualOperation::message_hold(const char *msg, int delay_in_s)
{
  if (mode == VISION) {
    message(msg);
  } else {
    message(msg);
    count = -1 * delay_in_s; // Estimate
  }
}

void ManualOperation::dump_images()
{
  message_hold("Saved images");
  image_input->dump_images();
}

void ManualOperation::long_input()
{
  clear();
  printw(
   "Input exact target attitude:\n"
   " speed <#> - set target speed to #\n"
   " yaw <#>   - set target yaw to #\n"
   " left <#>  - go left by # degress\n"
   " right <#> - go right by # degress\n"
   " depth <#> - set target depth to #\n"
   " up <#>    - go up by # cm\n"
   " down <#>  - go down by # cm\n"
   "\n"
   "Your input: "
  );

  refresh();

  char buf[BUF_LEN];
  int index = 0;

  char c = CharacterStreamSingleton::get_instance().wait_key(1000);
  while (c != '\r' && index < BUF_LEN - 1) {
    if (c != '\0' && c != -1) {
      const char BACKSPACE = 8;
      const char BACKSPACE2 = 127;
      if (c == BACKSPACE || c == BACKSPACE2) {
        if (index > 0) {
          index--;
          buf[index] = '\0';
          printw("%c %c", BACKSPACE, BACKSPACE);
        }
      } else {
        buf[index] = c;
        index++;
        printw("%c", c);
      }
      refresh();
    }
    c = CharacterStreamSingleton::get_instance().wait_key(1000);
  }

  buf[index] = '\0';

  printw("Your command: \n%s", buf);
  refresh();

  display_start_message();

  if (!strncmp("speed ", buf, strlen("speed "))) {
    int target_speed;
    sscanf(buf, "speed %d", &target_speed);
    actuator_output->set_attitude_absolute(SPEED, target_speed);
    message_hold("Target speed has been set");
  } else if (!strncmp("yaw ", buf, strlen("yaw "))) {
    int target_yaw;
    sscanf(buf, "yaw %d", &target_yaw);
    if (abs(target_yaw) <= 180) {
      actuator_output->set_attitude_absolute(YAW, target_yaw);
      message_hold("Target yaw has been set");
    } else {
      message_hold("Invalid yaw, must be [-180, 180]");
    }
  } else if (!strncmp("left ", buf, strlen("left "))) {
    int target_yaw_change;
    sscanf(buf, "left %d", &target_yaw_change);
    if (target_yaw_change >= 0 && target_yaw_change <= 180) {
      actuator_output->set_attitude_change(LEFT, target_yaw_change);
      message_hold("Turning left");
    } else {
      message_hold("Invalid left turn, must be [0, 180]");
    }
  } else if (!strncmp("right ", buf, strlen("right "))) {
    int target_yaw_change;
    sscanf(buf, "right %d", &target_yaw_change);
    if (target_yaw_change >= 0 && target_yaw_change <= 180) {
      actuator_output->set_attitude_change(RIGHT, target_yaw_change);
      message_hold("Turning right");
    } else {
      message_hold("Invalid left turn, must be [0, 180]");
    }
  } else if (!strncmp("depth ", buf, strlen("depth "))) {
    int target_depth;
    sscanf(buf, "depth %d", &target_depth);
    if (target_depth >= 0) {
      actuator_output->set_attitude_absolute(DEPTH, target_depth);
      message_hold("Target depth has been set");
    } else {
      message_hold("Invalid depth, must be >= 0");
    }
  } else if (!strncmp("up ", buf, strlen("up "))) {
    int target_depth_change;
    sscanf(buf, "up %d", &target_depth_change);
    if (target_depth_change >= 0) {
      actuator_output->set_attitude_change(RISE, target_depth_change);
      message_hold("Rising up");
    } else {
      message_hold("Invalid up command, must be >= 0");
    }
  } else if (!strncmp("down ", buf, strlen("down "))) {
    int target_depth_change;
    sscanf(buf, "down %d", &target_depth_change);
    if (target_depth_change >= 0) {
      actuator_output->set_attitude_change(SINK, target_depth_change);
      message_hold("Sinking down");
    } else {
      message_hold("Invalid down command, must be >= 0");
    }
  } else {
    message_hold("Invalid command");
  }
}
