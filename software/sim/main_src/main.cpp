/**
* @file sim/main.cpp
*
* @brief This is the main file for the both simulators
*
* This is the main file for simulator, calls init_sim() from site.cpp
*/

#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "bmp_io.h"
#include "types.h"
#include "physical_model.h"
#include "sim.h"

#include <highgui.h>

#include "motors.h"

// debug flags
unsigned DEBUG_MODEL = 0;

unsigned int randNum;   // global to determine fog thickness and site positions

/* physical model*/
physical_model model;

/**
* @brief Main loop for sim
*
* Open window with initial window size, title bar, color index display mode, and handle input events
*/
int main(int argc, char** argv)
{   
    printf (
    "\n   Welcome to the **OLD** Mechatronics Design Association Simulator!\n"
    "     This program is no longer supported... We thank you for your patronage.\n"
    "       --help:         print this message and exit\n"
    "       --rand:         randomize the fog density and obstacle positions\n"
    "       --debug-model:  print speed and accel of model\n"
    "\n   Commands: \n"
    "      wasd     -  accelerate fowards/back and turn on vertical axis\n"
    "      rf       -  accelerate rise/sink\n"
    "      e        -  passive stop (all accel to zero)\n"
    "      <space>  -  active stop\n"
    "      ijkl     -  translate fowards/back and turn on vertical axis\n"
    "      o;       -  translate rise/sink\n"
    "      p        -  save screenshot as cvSimImg.jpg\n\n"
    ); 
    
    randNum = 0;
    
    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "--help"))
            return (0);
        else if (!strcmp(argv[i], "--rand")) {
            srand (time(NULL));
            randNum = rand() % 15000;
        }
        else if (!strcmp(argv[i], "--debug-model"))
            DEBUG_MODEL = 1;
    }
   
   /*glut inits*/
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
   glutInitWindowSize (WINDOW_SIZE_X, WINDOW_SIZE_Y);
   glutInitWindowPosition(10, 0);
   glutCreateWindow ("Forwards Cam");
   init_sim();
   cv_init(); 
   
   /** register callback functions for glut */
   glutReshapeFunc  (cv_reshape);                    // called when window resized
   glutKeyboardFunc (cv_keyboard);                  // called with key pressed
   glutDisplayFunc  (cv_display);                    // called when glutPostRedisplay() raises redraw flag
   glutIdleFunc     (anim_scene);                        // called when idle (simulate speed)

    //cv_reshape (600, 400); 

    /*start the main glut loop*/
   glutMainLoop();
   
   destroy();
   return 0;
}
