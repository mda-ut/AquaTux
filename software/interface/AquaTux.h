#ifndef AQUATUX_H
#define AQUATUX_H

/*
* AquaTux reads a settings file and determines how to implement the following interfaces:
*   AttitudeInput:  Where to get readings for yaw, pitch, roll and depth (ie submarine IMU+depth sensor, simulator)
*   ImageInput:     Where to get a stream of input images (ie video, webcam, simulator)
*   Operation:      What algorithm (manual or autonomous) to control the output, given the inputs
*   ActuatorOutput: Where to output movement commands (ie change the simulator or give motor commands to the submarine)
*/

#include "AttitudeInput.h"
#include "ImageInput.h"
#include "Operation.h"
#include "ActuatorOutput.h"

class AquaTux {
  public:
    AquaTux(const char *);
    void work();
    ~AquaTux();
  private:
    AttitudeInput *m_attitude_input;
    ImageInput *m_image_input;
    Operation *m_operation;
    ActuatorOutput *m_actuator_output;
};

#endif
