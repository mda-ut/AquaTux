#include <GL/glut.h>
#include <time.h>
#include "bmp_io.h"
#include "types.h"
#include "physical_model.h"
#include "sim.h"

const float sky[4] = { .527343, .804687, 5/*1*/, 1.0f};
int clock_ticks_elapsed;
GLuint texName[10];

extern physical_model model;

/**
* @brief makes texture from given file
*
* @param filename
* File name
* @param tex_name
* Texture name, which is a number
*/
void makeTextureImage(char filename[], GLuint tex_name) {
   unsigned long width=0;
   long height=0;   /* width/height of textures */
   GLubyte *image=NULL; /* storage for texture */

   if (bmp_read (filename, &width, &height, image))
      assert(false);

   /* create texture object */
   glBindTexture(GL_TEXTURE_2D, texName[tex_name]);

   /* set up wrap & filter params */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   /*  printf("Texture %s, height %d, width %d\n",
       filename, (int)height, (int)width); */

   /* load the image */
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                0, GL_RGB, GL_UNSIGNED_BYTE, image);

   delete []image;
}

/**
* @brief setup camera based on the value of the model's angle and position
*/
void set_camera() {
   /* rotate scene */
   quat_camera(model.angle.roll, model.angle.pitch, model.angle.yaw);

   /* allow for translate scene */
   glTranslatef(-model.position.x, -model.position.y, -model.position.z);
   if (VERBOSE_CAMERA)
   {
      printf ("%d\n", VERBOSE_CAMERA);
      printf("position %f %f %f\n",
             model.position.x, model.position.y, model.position.z);
      printf("angle p(y) %f y(z) %f r(x) %f\n",
             model.angle.pitch, model.angle.yaw, model.angle.roll);
   }
}

/**
* @brief simulates speed (fwd, depth, angular yaw only)
*/
void anim_scene() 
{
   unsigned t = clock();
   float delta_t = (float)(t - clock_ticks_elapsed) / CLOCKS_PER_SEC; // in seconds
      
   if (delta_t > FRAME_DELAY)
   {
      model.update((float)delta_t);
      glutPostRedisplay();
      clock_ticks_elapsed = t;
   }
}

/**
* @brief Read textures and define lighting
*
* @see init_site
*/
//http://www.bme.jhu.edu/~reza/book/kinematics/kinematics.htm
// x is pointing right
// y is pointing up
// z is pointing towards the programmer
void init_sim() {
   glEnable (GL_LINE_SMOOTH);
   glHint (GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
   glLineWidth (1.5);
   
   model.reset_pos();
   model.reset_angle();
   model.angle.pitch = 0;

   /* set background colour */
   glClearColor(sky[0], sky[1], sky[2], sky[3]);

   /* generate texture names */
   texName[0] = 0;
   texName[1] = 1;
   texName[2] = 2;
   texName[3] = 3;
   texName[4] = 4;
   texName[5] = 5;
   texName[6] = 6;
   texName[7] = 7;
   texName[8] = 8;
   texName[9] = 9;
   glGenTextures(10, texName);

   /* load textures */
    //#define TNAME "img/green_resized.bmp"
    #define TNAME "img/floor2.bmp"
    #define SNAME "img/sky.bmp"
    #define SSNAME "img/surfsky.bmp"
    #define JVNAME "img/jamesvelcro.bmp"
    #define BINNAME1 "img/ship.bmp"
    #define BINNAME2 "img/tank.bmp"
    #define BINNAME3 "img/plane.bmp"
    #define BINNAME4 "img/factory.bmp"
    #define RED_CUTOUT "img/torpedo_cutout.bmp"
    #define BLUE_CUTOUT "img/torpedo_cutout.bmp"

   makeTextureImage((char *)TNAME, 0);
   makeTextureImage((char *)SNAME, 1);
   makeTextureImage((char *)SSNAME, 2);
   makeTextureImage((char *)JVNAME, 3);
   makeTextureImage((char *)BINNAME1, 4);
   makeTextureImage((char *)BINNAME2, 5);
   makeTextureImage((char *)BINNAME3, 6);
   makeTextureImage((char *)BINNAME4, 7);
   makeTextureImage((char *)RED_CUTOUT, 8);
   makeTextureImage((char *)BLUE_CUTOUT, 9);

   /* enable texturing & set texturing function */
   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   //http://www.falloutsoftware.com/tutorials/gl/gl8.htm
   GLfloat mat_specular[]={1.0, 1.0, 1.0, 1.0};
   GLfloat mat_diffuse[]={1.0, 1.0, 1.0, 1.0};
   GLfloat mat_ambient[]={1.0, 1.0, 1.0, 1.0};
   GLfloat mat_shininess={100.0};
   GLfloat light_ambient[]={0.0, 0.0, 0.0, 1.0};
   GLfloat light_diffuse[]={1.0, 1.0, 1.0, 1.0};
   GLfloat light_specular[]={1.0, 1.0, 1.0, 1.0};
   GLfloat light_position[]={0, 0.1, 0.0, 0.0};
   GLfloat cutoff[]={180};
   GLfloat light_direction[]={0, 1, 0.0};

   glLightfv(GL_LIGHT0, GL_POSITION, light_position);
   glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
   // don't shine on the floor
   glLightfv(GL_LIGHT0, GL_SPOT_CUTOFF, cutoff);
   glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION,light_direction);

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
   glMaterialf(GL_FRONT, GL_SHININESS, mat_shininess);

   glShadeModel(GL_SMOOTH);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);

   light_direction[2] = -1;
   glLightfv(GL_LIGHT1, GL_SPOT_CUTOFF, cutoff);
   glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION,light_direction);
   light_position[0]=5;
   light_position[1]=2;
   light_position[2]=-5;

   GLfloat light1_ambient[]={1.0, 1.0, 1.0, 1.0};
   GLfloat light1_diffuse[]={1.0, .65, .0, 1.0};
   GLfloat light1_specular[]={1.0, .65, .0, 1.0};

   glLightfv(GL_LIGHT1, GL_POSITION, light_position);
   glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
   glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
   glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
   glEnable(GL_LIGHT1);

   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);

   glEnable (GL_COLOR_MATERIAL);
   glColorMaterial (GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

   init_fog();
   init_site();
   
   clock_ticks_elapsed = clock();
}

void destroy () {  
    destroy_site ();
    glDeleteTextures(10, texName);
}
