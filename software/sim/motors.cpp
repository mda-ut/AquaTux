#include "motors.h"
#include <math.h>

void Motors::move(MOVE_DIR dir, float accel)
{
   switch(dir) {
   case FORWARD:
      model->accel += accel*FWD_ACCEL_CONST;
      break;
   case REVERSE:
      model->accel -= accel*FWD_ACCEL_CONST;
      break;
   case RISE:
      model->depth_accel += accel*DEPTH_ACCEL_CONST;
      break;
   case SINK:
      model->depth_accel -= accel*DEPTH_ACCEL_CONST;
      break;
   case RIGHT:
      model->angular_accel += accel*ANG_ACCEL_CONST;
      break;
   case LEFT:
      model->angular_accel -= accel*ANG_ACCEL_CONST;
      break;
   case STOP:
      model->accel = 0;
      model->depth_accel = 0;
      model->angular_accel = 0;
      break;
   }
   
   cap_accel ();
}

void Motors::translate(MOVE_DIR dir)
{
    switch(dir) {
        case FORWARD:
            model->position.z -= POS_INC*cos((model->angle.yaw*PI)/180)*cos((model->angle.pitch*PI)/180);
            model->position.x += POS_INC*sin((model->angle.yaw*PI)/180)*cos((model->angle.pitch*PI)/180);
            model->position.y -= POS_INC*sin((model->angle.pitch*PI)/180);
            break;
        case REVERSE:
            model->position.z += POS_INC*cos((model->angle.yaw*PI)/180)*cos((model->angle.pitch*PI)/180);
            model->position.x -= POS_INC*sin((model->angle.yaw*PI)/180)*cos((model->angle.pitch*PI)/180);
            model->position.y += POS_INC*sin((model->angle.pitch*PI)/180);
            break;
        case RISE:
            model->position.y += POS_INC;
            break;
        case SINK:
            model->position.y -= POS_INC;
            break;
        case RIGHT:
            model->angle.yaw += ANGLE_INC/2;
            break;
        case LEFT:
            model->angle.yaw -= ANGLE_INC/2;
            break;
        case STOP:
            model->speed = 0;
            model->depth_speed = 0;
            model->angular_speed = 0;
            break;
    }
}

void Motors::key_command (char key) {
    switch (key) {
        case 'w':
            move (FORWARD);
            break;
        case 's':
            move (REVERSE);
            break;
        case 'a':
            move (LEFT);
            break;
        case 'd':
            move (RIGHT);
            break;
        case 'r':
            move (RISE);
            break;
        case 'f':
            move (SINK);
            break;
        // translations
        case 'i': // forwards/back and turn is wasd
            translate(FORWARD);
            break;
        case 'k':
            translate(REVERSE);
            break;
        case 'j':
            translate(LEFT);
            break;
        case 'l':
            translate(RIGHT);
            break;
        case 'o':
            translate(RISE);
            break;
        case ';':
            translate(SINK);
            break;
        case ' ':
            move(STOP);
            break;
        default:
            model->accel = 0;
            model->depth_accel = 0;
            model->angular_accel = 0;
            break;
    }
}

void Motors::cap_accel () {
    if (model->accel > FWD_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->accel = FWD_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
    if (model->accel < -FWD_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->accel = -FWD_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
    if (model->depth_accel > DEPTH_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->depth_accel = DEPTH_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
    if (model->depth_accel < -DEPTH_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->depth_accel = -DEPTH_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
    if (model->angular_accel > ANG_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->angular_accel = ANG_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
    if (model->angular_accel < -ANG_ACCEL_CONST*ACCEL_LEVEL_LIMIT) 
        model->angular_accel = -ANG_ACCEL_CONST*ACCEL_LEVEL_LIMIT;
}
