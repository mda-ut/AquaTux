/**
* @file mda2009/sim/types.h
*
* @brief Contains all the constants and data structures for the underwater sim
*
* Serves as a header for .cpp files without headers
*/

#define LONGV

#ifndef MATH_TYPES_GL_H__
#define MATH_TYPES_GL_H__

// CIF format
// http://fr.wikipedia.org/wiki/QCIF
#define WINDOW_SIZE_X 352
#define WINDOW_SIZE_Y 288

/** Current sub position in the sim*/
typedef struct
{
   float x;
   float y;
   float z;
} world_vector;

/** Sub orientation in the sim*/
typedef struct
{
   float pitch; /* spin around x axis in degrees */
   float yaw;   /* spin around y axis in degrees */
   float roll;  /* spin around z axis in degrees */
} orientation;

enum SPEED_DIR {
   FORWARD_DIR = 0,
   REVERSE_DIR,
   UP_DIR,
   DOWN_DIR,
   POS_ROT,
   NEG_ROT
};

// for site.cpp
void init_fog();
void init_site();
void draw();
void destroy_site();

// for main.cpp
//void screenshot();

// for glQuaternion.cpp
void quat_camera(float roll, float pitch, float yaw);

#endif

