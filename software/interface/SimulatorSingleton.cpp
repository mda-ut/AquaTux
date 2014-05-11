#include <GL/glut.h>
#include <GL/freeglut_ext.h>

#include "CharacterStreamSingleton.h"
#include "SimulatorSingleton.h"
#include "mgui.h"
#include "sim.h"

#define POOL_HEIGHT 8

#define P_FACTOR 0.2
#define I_FACTOR 0.05
#define D_FACTOR 0.05
#define A_FACTOR 0.75
#define DEPTH_FACTOR 2.5f
#define MAX_YAW 1.f
#define MAX_DEPTH 1.f

#define SIM_SETTINGS_FILE "sim.csv"

// Global variables needed by sim
volatile physical_model model;
unsigned DEBUG_MODEL = 0;
unsigned int randNum;

// Function declarations
void *run_sim(void *);
void sim_init();
void sim_keyboard(unsigned char, int, int);
void sim_display();
void sim_reshape(int, int);
void sim_idle();
void sim_close_window();

/* Constructor and destructor for sim resource */

SimulatorSingleton::SimulatorSingleton() : registered(false), created(false), thread_done(false),
    img_fwd(NULL), img_dwn(NULL), img_copy_start(false), img_copy_done(false),
    PID_yaw (P_FACTOR, I_FACTOR, D_FACTOR, A_FACTOR),
    PID_depth (1,0,0,0)
{
}

SimulatorSingleton::~SimulatorSingleton()
{
  destroy();
}

void SimulatorSingleton::create()
{
  if (!registered) {
    return;
  }
  created = true;

  // initialize barrier
  sem_init(&sema, 0, 0);

  // initialize target to the model
  target_model = *const_cast<physical_model *>(&model);

  // Start simulation (may need to use passed in data)
  pthread_create(&sim_thread, NULL, ::run_sim, NULL);

  // Wait until sim has initialized
  sem_wait(&sema);
  sem_destroy(&sema);
}

void SimulatorSingleton::destroy()
{
  if (!created) {
    return;
  }

  thread_done = true;
  pthread_join(sim_thread, NULL);

  cvReleaseImage(&img_fwd);
  cvReleaseImage(&img_dwn);
}

/* Accessors */

IplImage* SimulatorSingleton::get_image(ImageDirection dir)
{
  img_dir = dir;
  img_copy_start = true;
  img_copy_done = false;
  sim_idle();

  // Wait until transfer complete
  while(!img_copy_done) {
    ;
  }

  return (dir == FWD_IMG) ? img_fwd : img_dwn;
}

physical_model SimulatorSingleton::attitude()
// Returns a copy of the model.
//
// Since model is volatile, it must be copied (memcpy is safe to cast away the volatile pointer)
{
  physical_model m;
  memcpy(&m, (char *)&model, sizeof(physical_model));
  m.position.y = POOL_HEIGHT - m.position.y;
  return m;
}

/* Mutators */

void SimulatorSingleton::add_position(world_vector p)
{
  model.position.x += p.x;
  model.position.y += p.y;
  model.position.z += p.z;

  set_target_depth(POOL_HEIGHT - model.position.y);
}

void SimulatorSingleton::add_orientation(orientation a)
{
  model.angle.yaw += a.yaw;
  model.angle.pitch += a.pitch;
  model.angle.roll += a.roll;

  set_target_yaw(model.angle.yaw);
}

#define ACCEL_FACTOR .4
void SimulatorSingleton::set_target_accel(float accel)
{
  // No need to use target, set directly on the model
  model.accel = ACCEL_FACTOR * accel;
}

void SimulatorSingleton::set_target_yaw(float yaw)
{
  target_model.angle.yaw = yaw;
  if (target_model.angle.yaw >= 180) {
    target_model.angle.yaw -= 360;
  } else if (target_model.angle.yaw < -180) {
    target_model.angle.yaw += 360;
  }
}

void SimulatorSingleton::set_target_depth(float depth)
{
  target_model.position.y = POOL_HEIGHT - depth;
}

// yaw in degrees, depth in cm
void SimulatorSingleton::set_target_attitude_change(float yaw, float depth)
{
  target_model.angle.yaw = model.angle.yaw + yaw;
  if (target_model.angle.yaw >= 180) {
    target_model.angle.yaw -= 360;
  } else if (target_model.angle.yaw < -180) {
    target_model.angle.yaw += 360;
  }
  target_model.position.y = model.position.y + depth / 100;
}

void SimulatorSingleton::zero_speed()
{
  model.speed = 0;
  model.depth_speed = 0;
  model.angular_speed = 0;

  model.accel = 0;
  model.angular_accel = 0;
  model.depth_accel = 0;

  set_target_yaw(model.angle.yaw);
  set_target_depth(POOL_HEIGHT - model.position.y);
}

int SimulatorSingleton::target_yaw()
{
  return target_model.angle.yaw;
}

// On the order of cm
int SimulatorSingleton::target_depth()
{
  return (POOL_HEIGHT - target_model.position.y) * 100;
}

void *run_sim(void *args)
{
  SimulatorSingleton::get_instance().run_sim();
  return NULL;
}

void SimulatorSingleton::run_sim()
{
  int argc = 0;
  char *argv[1];
  unsigned width, height;
  read_common_mv_setting ("IMG_WIDTH_COMMON", width);
  read_common_mv_setting ("IMG_HEIGHT_COMMON", height);
  
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize (width, height);
  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
  glutInitWindowPosition(10, 0);

  dwn_window = glutCreateWindow ("Down Cam");
  glutPositionWindow(0, 400);
  glutReshapeFunc  (::sim_reshape);
  glutDisplayFunc  (::sim_display);
  glutIdleFunc     (::sim_idle);
  glutKeyboardFunc (::sim_keyboard);
  glutCloseFunc    (::sim_close_window);
  init_sim();

  fwd_window = glutCreateWindow ("Forwards Cam");
  glutReshapeFunc  (::sim_reshape);
  glutDisplayFunc  (::sim_display);
  glutIdleFunc     (::sim_idle);
  glutKeyboardFunc (::sim_keyboard);
  glutCloseFunc    (::sim_close_window);
  init_sim();

  img_fwd = cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 3);
  img_dwn = cvCreateImage (cvSize(width,height), IPL_DEPTH_8U, 3);

  // set initial position
  std::string start_at;
  read_mv_setting(SIM_SETTINGS_FILE, "START_AT", start_at);

  float start_x = 0.f;
  float start_y = 0.f;
  float start_z = 0.f;
  float start_yaw = 0.f;

  if (start_at == "GATE") {
    start_x = -3.f;
    start_y = 7.5f;
    start_z = 16.f;
  } else if (start_at == "BUOY") {
    start_x = -3.3f;
    start_y = 4.5f;
    start_z = 3.f;
    start_yaw = 18.f;
  } else if (start_at == "FRAME") {
    start_x = 1.f;
    start_y = 2.5f;
    start_z = -4.2f;
    start_yaw = 80.f;
  } else if (start_at == "MULTIPATH") {
    start_x = 7.f;
    start_y = 2.5f;
    start_z = -5.f;
    start_yaw = 80.f;
  } else if (start_at == "TORPEDO_TARGET") {
    start_x = 10.1f;
    start_y = 2.35f;
    start_z = -1.9f;
    start_yaw = -170.f;
  } else if (start_at == "MARKER_DROPPER") {
    start_x = 10.8f;
    start_y = 2.5f;
    start_z = -7.1f;
    start_yaw = -4.f;
  }

  model.position.x = start_x;
  model.position.y = start_y;
  model.position.z = start_z;
  model.angle.yaw = start_yaw;

  set_target_depth(POOL_HEIGHT - start_y);
  set_target_yaw(start_yaw);

  // Sim has initialized
  sem_post(&sema);

  glutMainLoop();
}

void sim_keyboard(unsigned char key, int x, int y)
{
  SimulatorSingleton::get_instance().sim_keyboard(key);
}

void SimulatorSingleton::sim_keyboard(unsigned char key)
{
  CharacterStreamSingleton::get_instance().write_char(key);
}

void sim_display()
{
  SimulatorSingleton::get_instance().sim_display();
}

void SimulatorSingleton::sim_display()
{
  if (thread_done) {
    glutLeaveMainLoop();
    ::destroy();
  }

  if (img_copy_start) {
    img_copy_start = false;
    if (img_dir == DWN_IMG) {
      model.angle.pitch += 90; // grab downwards
    }
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    set_camera();
    draw();
    if (img_dir == FWD_IMG) {
      cvQueryFrameGL (img_fwd);
    } else {
      cvQueryFrameGL (img_dwn);
    }
    if (img_dir == DWN_IMG) {
      model.angle.pitch -= 90; // reset pitch
    }
    img_copy_done = true;
  } else {
    // front camera
    glutSetWindow(fwd_window);
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    set_camera();
    draw();
    glutSwapBuffers();

    // down camera
    glutSetWindow(dwn_window);
    model.angle.pitch += 90;
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    set_camera();
    draw();
    glutSwapBuffers();   
    model.angle.pitch -= 90;
  }

  glFlush();
}

void sim_reshape(int w, int h)
{
  SimulatorSingleton::get_instance().sim_reshape(w, h);
}

void SimulatorSingleton::sim_reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(CAMERA_FIELD_OF_VIEW, (GLfloat)w/(GLfloat)h, .05, 200.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  if (w == img_fwd->width && h == img_fwd->height) {
    return;
  }
   
  // Also resize the Ipl images
  cvReleaseImage (&img_fwd);
  img_fwd = cvCreateImage (cvSize(w,h), IPL_DEPTH_8U, 3);

  cvReleaseImage (&img_dwn);
  img_dwn = cvCreateImage (cvSize(w,h), IPL_DEPTH_8U, 3);
}

void sim_idle()
{
  SimulatorSingleton::get_instance().sim_idle();
}


void SimulatorSingleton::sim_idle()
{
  double d_yaw = target_model.angle.yaw - model.angle.yaw;
  if (d_yaw > 180) {
    d_yaw -= 360;
  } else if (d_yaw < -180) {
    d_yaw += 360;
  }

  PID_depth.PID_Update (target_model.position.y - model.position.y);
  PID_yaw.PID_Update (d_yaw);
  //printf("%f\n", PID_yaw.PID_Output());
  // cast away volatile model
  physical_model temp_model = *const_cast<physical_model *>(&model);

  //Clamp the output
  temp_model.angular_accel  = std::min(PID_yaw.PID_Output(), MAX_YAW);
  temp_model.depth_accel    = std::min(PID_depth.PID_Output() * DEPTH_FACTOR, MAX_DEPTH);

  temp_model.angular_accel  = std::max(temp_model.angular_accel, -MAX_YAW);
  temp_model.depth_accel    = std::max(temp_model.depth_accel,   -MAX_DEPTH);

  // copy back to volatile model
  memcpy((void *)&model, &temp_model, sizeof(temp_model));

  ::anim_scene();
}

void sim_close_window()
{
  SimulatorSingleton::get_instance().sim_close_window();
}

void SimulatorSingleton::sim_close_window()
{
  if (!thread_done) {
    CharacterStreamSingleton::get_instance().write_char('q');
  }
}
