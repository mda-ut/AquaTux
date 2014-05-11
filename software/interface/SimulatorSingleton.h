/* SimulatorSingleton

  This class implements a singleton as an interface to the simulator. The simulator should only have one instance, but can
  be accessed by different objects, hence the singleton. The actual simulator is run in a different thread, using shared global
  variables to pass data. The Simulator can return a set of images, attitude information and can be controlled by setting a target
  orientation.
*/

#ifndef SIMULATOR_SINGLETON_H
#define SIMULATOR_SINGLETON_H

#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#include "physical_model.h"

#include "ImageInput.h"
#include "PID.h"

class SimulatorSingleton {
  public:
    SimulatorSingleton();
    ~SimulatorSingleton();

    static SimulatorSingleton& get_instance()
    {
      static SimulatorSingleton instance;
      return instance;
    }
    void register_instance()
    {
      if (registered) {
        return;
      }
      registered = true;
      create();
    }

    void run_sim();
    void sim_keyboard(unsigned char);
    void sim_display();
    void sim_reshape(int, int);
    void sim_idle();
    void sim_close_window();
    IplImage* get_image(ImageDirection);

    physical_model attitude();
    void add_position(world_vector);
    void add_orientation(orientation);
    void set_target_accel(float);
    void set_target_yaw(float);
    void set_target_depth(float);
    void set_target_attitude_change(float, float);
    void zero_speed();
    int target_yaw();
    int target_depth();
  private:
    SimulatorSingleton(SimulatorSingleton const&); // Don't implement
    void operator=(SimulatorSingleton const&);     // Don't implement

    void create();
    void destroy();

    bool registered, created, thread_done;
    pthread_t sim_thread;
    sem_t sema;
    IplImage *img_fwd, *img_dwn;
    volatile bool img_copy_start, img_copy_done;
    volatile ImageDirection img_dir;
    int fwd_window, dwn_window;

    physical_model target_model;

    PID PID_yaw;
    PID PID_depth;
};

#endif
