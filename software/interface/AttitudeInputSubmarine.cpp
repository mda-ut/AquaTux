#include "AttitudeInput.h"
#include "SubmarineSingleton.h"
#include "../scripts/scripts.h"

AttitudeInputSubmarine::AttitudeInputSubmarine()
{
  SubmarineSingleton::get_instance().register_instance();
}

AttitudeInputSubmarine::~AttitudeInputSubmarine()
{
}

// In degrees
int AttitudeInputSubmarine::yaw()
{
  return get_yaw();
}

// On the order of cm
int AttitudeInputSubmarine::depth()
{
  return get_depth();
}

int AttitudeInputSubmarine::target_yaw()
{
  return SubmarineSingleton::get_instance().get_target_yaw();
}

int AttitudeInputSubmarine::target_depth()
{
  return SubmarineSingleton::get_instance().get_target_depth();
}
