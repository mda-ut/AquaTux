#ifndef _PHYSICALMODEL_H
#define _PHYSICALMODEL_H

#include "types.h"

#define ANGLE_INC 5.
#define POS_INC .2
#define PI 3.14159265

#define DEPTH_SPEED_SCALING 0.02
#define FWD_SPEED_SCALING 0.06
#define SIDE_SPEED_SCALING 0.4

#define FWD_LOSS_CONST 0.06
#define DEPTH_LOSS_CONST 0.05
#define ANG_LOSS_CONST 0.05

class physical_model
{
public:
   world_vector position;
   orientation angle;
   float speed, depth_speed, angular_speed;
   float accel, depth_accel, angular_accel;

public:
   physical_model();
   physical_model(float x, float y, float z);
   ~physical_model();
   void update(float delta_t);
   void reset_angle();
   void reset_pos();
   void reset_speed();
   void reset_accel();
   void print();
};
#endif
