#include "AttitudeInput.h"
#include "SimulatorSingleton.h"
#include "physical_model.h"

AttitudeInputSimulator::AttitudeInputSimulator()
{
  SimulatorSingleton::get_instance().register_instance();
}

AttitudeInputSimulator::~AttitudeInputSimulator()
{
}

// yaw, pitch and roll are in degrees
int AttitudeInputSimulator::yaw()
{
  physical_model model = SimulatorSingleton::get_instance().attitude();
  return (int)model.angle.yaw;
}

int AttitudeInputSimulator::pitch()
{
  physical_model model = SimulatorSingleton::get_instance().attitude();
  return (int)model.angle.pitch;
}

int AttitudeInputSimulator::roll()
{
  physical_model model = SimulatorSingleton::get_instance().attitude();
  return (int)model.angle.roll;
}

// On the order of cm
int AttitudeInputSimulator::depth()
{
  physical_model model = SimulatorSingleton::get_instance().attitude();
 
  return (int)(100*(model.position.y));
}

int AttitudeInputSimulator::target_yaw()
{
  return SimulatorSingleton::get_instance().target_yaw();
}

int AttitudeInputSimulator::target_depth()
{
  return SimulatorSingleton::get_instance().target_depth();
}
