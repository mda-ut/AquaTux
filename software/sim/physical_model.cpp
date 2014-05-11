#include <math.h>
#include "physical_model.h"
#include <stdio.h>

// initial position and angle
#include "init.h"

physical_model::physical_model()
{
   // on top of a box
   reset_angle();
   reset_pos();
   reset_speed();
};

physical_model::physical_model(float x, float y, float z)
{
   // simulator coordinates
   position.x = x;
   position.y = y;
   position.z = z;
}

physical_model::~physical_model()
{
}

void physical_model::reset_angle()
{
   angle.pitch = A_P;
   angle.yaw = A_Y;
   angle.roll = A_R;
}

void physical_model::reset_pos()
{
   position.x = REF_X;
   position.y = REF_Y;
   position.z = REF_Z;
}

void physical_model::reset_speed()
{
   speed = 0;
   depth_speed = 0;
   angular_speed = 0;
}

void physical_model::reset_accel()
{
   accel = 0;
   depth_accel = 0;
   angular_accel = 0;
}

void physical_model::print()
{
   printf ("speed: %6.2f\t", speed);
   printf ("depth_speed: %6.2f\t", depth_speed);
   printf ("ang_speed: %6.2f\n", angular_speed); 
   printf ("accel: %6.2f\t", accel);
   printf ("depth_accel: %6.2f\t", depth_accel);
   printf ("ang_accel: %6.2f\n\n", angular_accel); 
}

void range_angle (float& angle)
{
   if (angle >= 180)        angle -= 360;
   else if (angle <= -180)  angle += 360;
}
float friction_cap (float frictional_accel)
{
   if (frictional_accel >= 0.5) return 0.5;
   if (frictional_accel <= -0.5) return -0.5;
   return frictional_accel;
}

// time past since last iteration in seconds
void physical_model::update(float delta_t)
{
    speed +=  accel - FWD_LOSS_CONST*speed;
    depth_speed += depth_accel - DEPTH_LOSS_CONST*depth_speed;
    angular_speed += angular_accel - ANG_LOSS_CONST*angular_speed;
    
    //if (fabs(speed) < 0.1) speed = 0;
    //if (fabs(depth_speed) < 0.1) depth_speed = 0;
    //if (fabs(angular_speed) < 0.1) angular_speed = 0;

    float distance_traveled = speed * delta_t * FWD_SPEED_SCALING;
        
    position.x = position.x + sin(angle.yaw * M_PI/180) * distance_traveled;
    position.z = position.z - cos(angle.yaw * M_PI/180) * distance_traveled;
    position.y += depth_speed * delta_t * DEPTH_SPEED_SCALING;

    float dtheta = angular_speed * delta_t * SIDE_SPEED_SCALING;    
    angle.yaw += dtheta;
    range_angle (angle.yaw);
}
