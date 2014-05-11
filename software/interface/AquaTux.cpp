#include <iostream>

#include "AquaTux.h"
#include "Mission.h"
#include "SimulatorSingleton.h"
#include "SubmarineSingleton.h"
#include "mgui.h"

using namespace std;

AquaTux::AquaTux(const char *settings_file) : m_attitude_input(NULL)
{
  // Read settings for input, control algorithm and output classes
  string attitude_input, image_input, operation, actuator_output;
  read_mv_setting(settings_file, "ATTITUDE_INPUT", attitude_input);
  read_mv_setting(settings_file, "IMAGE_INPUT", image_input);
  read_mv_setting(settings_file, "OPERATION", operation);
  read_mv_setting(settings_file, "ACTUATOR_OUTPUT", actuator_output);

  // Attitude input
  if (attitude_input == "SIMULATOR") {
    m_attitude_input = new AttitudeInputSimulator();
  } else if (attitude_input == "SUBMARINE") {
    m_attitude_input = new AttitudeInputSubmarine();
  } else {
    if (attitude_input != "NULL") {
      cout << "Warning: unrecognized attitude input " << attitude_input << ", defaulting to no attitude input\n";
    }
    m_attitude_input = new AttitudeInputNull();
  }

  // Image input
  if (image_input == "SIMULATOR") {
    m_image_input = new ImageInputSimulator(settings_file);
  } else if (image_input == "VIDEO") {
    m_image_input = new ImageInputVideo(settings_file);
  } else if (image_input == "WEBCAM") {
    m_image_input = new ImageInputWebcam(settings_file);
  } else {
    if (image_input != "NULL") {
      cout << "Warning: unrecognized image input " << image_input << ", defaulting to no image input\n";
    }
    m_image_input = new ImageInputNull();
  }

  // Actuator output
  if (actuator_output == "SIMULATOR") {
    m_actuator_output = new ActuatorOutputSimulator();
  } else if (actuator_output == "SUBMARINE") {
    ActuatorOutputSubmarine *aos = new ActuatorOutputSubmarine();

    // Set PID constants
    double P, I, D, Alpha;
    read_mv_setting(settings_file, "DEPTH_P", P);
    read_mv_setting(settings_file, "DEPTH_I", I);
    read_mv_setting(settings_file, "DEPTH_D", D);
    read_mv_setting(settings_file, "DEPTH_ALPHA", Alpha);
    aos->set_depth_constants(P, I, D, Alpha);

    read_mv_setting(settings_file, "PITCH_P", P);
    read_mv_setting(settings_file, "PITCH_I", I);
    read_mv_setting(settings_file, "PITCH_D", D);
    read_mv_setting(settings_file, "PITCH_ALPHA", Alpha);
    aos->set_pitch_constants(P, I, D, Alpha);

    read_mv_setting(settings_file, "ROLL_P", P);
    read_mv_setting(settings_file, "ROLL_I", I);
    read_mv_setting(settings_file, "ROLL_D", D);
    read_mv_setting(settings_file, "ROLL_ALPHA", Alpha);
    aos->set_roll_constants(P, I, D, Alpha);

    read_mv_setting(settings_file, "YAW_P", P);
    read_mv_setting(settings_file, "YAW_I", I);
    read_mv_setting(settings_file, "YAW_D", D);
    read_mv_setting(settings_file, "YAW_ALPHA", Alpha);
    aos->set_yaw_constants(P, I, D, Alpha);

    m_actuator_output = aos;
  } else {
    if (actuator_output != "NULL") {
      cout << "Warning: unrecognized actuator output " << actuator_output << ", defaulting to no actuator output\n";
    }
    m_actuator_output = new ActuatorOutputNull();
  }

  // Operation, ie overall algorithm
  if (operation == "MANUAL") {
    m_operation = new ManualOperation(m_attitude_input, m_image_input, m_actuator_output);
  } else if (operation == "MISSION") {
    m_operation = new Mission(m_attitude_input, m_image_input, m_actuator_output);
  } else {
    if (operation != "NULL") {
      cout << "Warning: unrecognized operation algorithm " << operation << ", defaulting to no operation algorithm\n";
    }
    m_operation = new OperationNull(m_attitude_input, m_image_input, m_actuator_output);
  }
}

void AquaTux::work()
{
  // start the control algorithm until mission completes
  m_operation->work();
}

AquaTux::~AquaTux()
{
  delete m_attitude_input;
  delete m_image_input;
  delete m_actuator_output;
  delete m_operation;
}
