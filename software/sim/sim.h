#ifndef _SIM_COMMON_H_
#define _SIM_COMMON_H_

#define VERBOSE_CAMERA 0 // print lots of random stuff if 1
#define FRAME_DELAY 0.01 // max refresh rate in seconds

/* makes texture from given file, outputs textrue name as Gluint */
void makeTextureImage(char filename[], GLuint tex_name);
/* setup camera */
void set_camera();
/* simulates speed */
void anim_scene(); 
/* reads textures and define lighting */
void init_sim();
/* destroys defined objects */
void destroy();

/** opencv related functions below */

#include <highgui.h>

#define CAMERA_FIELD_OF_VIEW 52 //37

/** OpenCV integration section starts here.
 *  Info:
 *  The glut model of code goes revolves around the function glutMainLoop(). A
 *  glut program generally has 2 steps: 
 *      1. Init the main loop with parameters. 
 *      2. Register functions that will be called by mainloop when keyboard is hit,
 *      screen needs to be redraw, etc
 *      3. Call glutMainLoop. Once this is done the loop cannot be exited.
 * 
 *  For our purposes theres only 2 functions you need to care about.
 *      cv_display - function that is called whenever sim needs to update the
 *          window. redraws the window.
 *      cv_keyboard - function that is called whenever key is hit. It processes
 *          the keyhit, updates the model, and raises a flag to redraw window.
 *  Thus everytime u hit key the 2 above functions get called. Idle redraw has been
 *      disabled for now.
 * 
 * The openCV integrated model will expand this: instead of 
 *      key hit -> glut updates model using key -> update parameters -> redraw window
 * We have: 
 *      Any key hit -> send current img to opencv -> cv does whatever with the img 
 *      -> glut updates model using key -> redraw window
 * 
 * The Any key hit refers to the user hitting any key. Its just to "go to next step" of sim.
 * User has no input when cv is running. cv will overwrite the user's key.
 * 
 * This is implemented by adding a section in cv_display that sends img to cv and processes
 * it. Thus cv will see a new image immediately as it is painted on window. 
 */

void cv_init ();
void cvQueryFrameGL (IplImage* img);
void cv_display ();
void cv_keyboard(unsigned char key, int x, int y);
void cv_reshape(int w, int h);

#endif
