/**
* @file mda2009/sim/site.cpp
*
* @brief Creates the site for the underwater sim
*
* Creates all site elements for the underwater sim
*/

#include <GL/glut.h>
#include "types.h"
#include "PID.h" // for the gettimeofday wrapper

#define CYCLE_COLORS
#define NUM_BUOYS 3
#define EXIT_SIDES 8
#define WINDOW_CUTOUT_PARTS 1
#define GOALPOST_PARTS 4

int list;
GLUquadricObj *g1/**Side gate post*/, *g2/**Other side gate post*/, *g3/**Top gate post*/, *buoys[2 * NUM_BUOYS], *EXIT[2 * EXIT_SIDES], *window_cutout[WINDOW_CUTOUT_PARTS], *goalpost[2 * GOALPOST_PARTS];
#define NLIST 4

extern GLuint texName[10];
extern unsigned int randNum;

#define FOG_PANEL 0

#define NUM_PANELS_FLOOR  5
#define NUM_BOARDS        8
#define NUM_PANELS_BOARDS 1

// floor is biased towards the back and the right
#define FLOOR_SIZE (40/2)
#define FLOOR_OFFSET 5
#define POOL_HEIGHT 8 //4.8768
#define PLEFT -FLOOR_SIZE
#define PRIGHT FLOOR_SIZE+FLOOR_OFFSET+4
#define PBACK -FLOOR_SIZE-FLOOR_OFFSET
#define PFRONT FLOOR_SIZE
#define TEXTURE_SCALE 1

world_vector vertices[NUM_PANELS_FLOOR*4] =
{
// floor
   {PLEFT, 0.0, PBACK}, {PRIGHT, 0.0, PBACK}, {PRIGHT, 0.0, PFRONT}, {PLEFT, 0.0, PFRONT},
   // vert left
   {PLEFT, 0.0, PBACK}, {PLEFT, POOL_HEIGHT, PBACK}, {PLEFT, POOL_HEIGHT, PFRONT}, {PLEFT, 0.0, PFRONT},
   // vert right
   {PRIGHT, 0.0, PBACK}, {PRIGHT, POOL_HEIGHT, PBACK}, {PRIGHT, POOL_HEIGHT, PFRONT}, {PRIGHT, 0.0, PFRONT},
   // vert back
   {PLEFT, 0.0, PBACK}, {PRIGHT, 0.0, PBACK}, {PRIGHT, POOL_HEIGHT, PBACK}, {PLEFT, POOL_HEIGHT, PBACK},
   // vert front
   {PLEFT, 0.0, PFRONT}, {PRIGHT, 0.0, PFRONT}, {PRIGHT, POOL_HEIGHT, PFRONT}, {PLEFT, POOL_HEIGHT, PFRONT},
};

world_vector ceiling[1*4] =
{
   {PLEFT, POOL_HEIGHT, PBACK}, {PRIGHT, POOL_HEIGHT, PBACK}, {PRIGHT, POOL_HEIGHT, PFRONT}, {PLEFT, POOL_HEIGHT, PFRONT}
};

// aligned in the z direction
world_vector boards[NUM_PANELS_BOARDS*4] =
{
   {-.0762, 0.5, -.6096},
   {.0762, 0.5, -.6096},
   {.0762, 0.5, .6096},
   {-.0762, 0.5, .6096}
};

world_vector buoys_v[NUM_BUOYS] =
{ // BUOY POSITIONS. XYZ, Y is HEIGHT, more -ve is higher, BOUYS ALONG Z
   {0, 3.6, -5.5},
   {0, 4, -4},
   {0, 3.0, -2.5}
};

#define NUM_BOXES 4
#define NUM_PANELS_BOXES 7
world_vector boxes[NUM_PANELS_BOXES*4] =
{
   // top
   {-.304800, 0.5, -.457200}, {.304800, 0.5, -.457200}, {.304800, 0.5, .457200}, {-.304800, 0.5, .457200},
   // bottom
   {-.304800, .3476, -.457200}, {.304800, .3476, -.457200}, {.304800, .3476, .457200}, {-.304800, .3476, .457200},

   // long sides
   {-.304800, .3476, -.457200}, {-.304800, .5, -.457200}, {-.304800, .5, .457200}, {-.304800, .3476, .457200},
   {.304800, .3476, -.457200}, {.304800, .5, -.457200}, {.304800, .5, .457200}, {.304800, .3476, .457200},

   // short sides
   {-.304800, .3476, -.457200}, {.304800, .3476, -.457200}, {.304800, .5, -.457200}, {-.304800, .5, -.457200},
   {-.304800, .3476, .457200}, {.304800, .3476, .457200}, {.304800, .5, .457200}, {-.304800, .5, .457200},

   // hole
   {-.1524, 0.501, -0.3048}, {.1524, 0.501, -0.3048}, {.1524, 0.501, 0.3048}, {-.1524, 0.501, 0.3048}
};

float b_rotations[NUM_BOARDS] = {35, -62, -10, -98, -35, -25, -75, -10};

float bx_rotations[NUM_BOXES] = {95, 95, 95, 95};

#include "planks.h"

#if FOG_PANEL
#define NUM_PANELS_FOG    1
world_vector f_vertices[NUM_PANELS_FOG*4] =
{
   {-5.0, -5.0, 0}, {5.0, -5.0, 0}, {5.0, 5.0, 0}, {-5.0, 5.0, 0}
};

/*world_vector normals[NUM_PANELS] = {
  {0.0, 1.0, 0.0}
  }; */
/* Function to set fog colour for the current vertex */

#endif

/*
  http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=16
  glFogi(GL_FOG_MODE, fogMode[fogfilter]); establishes the fog filter mode. Now earlier we declared the array fogMode. It held GL_EXP, GL_EXP2, and GL_LINEAR. Here is when these variables come into play. Let me explain each one:

  * GL_EXP - Basic rendered fog which fogs out all of the screen. It doesn't give much of a fog effect, but gets the job done on older PC's.
  * GL_EXP2 - Is the next step up from GL_EXP. This will fog out all of the screen, however it will give more depth to the scene.
  * GL_LINEAR - This is the best fog rendering mode. Objects fade in and out of the fog much better.
*/
GLuint  fogMode[]= { GL_EXP, GL_EXP2, GL_LINEAR }; // Storage For Three Types Of Fog
GLuint  fogfilter = 0;                             // Which Fog Mode To Use

GLfloat fogColor[4] = {.560784,.737254,.560784,1.0};   // Fog Color glColor3f ( 1.0f, 1.0f, 1.0f ) ;
void init_fog()
{

//  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really nice perspective calculations
   glFogi(GL_FOG_MODE, fogMode[fogfilter]);            // Fog Mode
   glFogfv(GL_FOG_COLOR, fogColor);                    // Set Fog Color
   glFogf(GL_FOG_DENSITY, 0.125f + randNum / 150000.); // How Dense Will The Fog Be (randomized)
   glHint(GL_FOG_HINT, GL_DONT_CARE);                  // Fog Hint Value
   glFogf(GL_FOG_START, 1.0f);                         // Fog Start Depth
   glFogf(GL_FOG_END, PBACK);                          // Fog End Depth
   glEnable(GL_FOG);                                   // Enables GL_FOG
}

/**
* @brief Defines gate location
*/
void do_posts();

/**
* @brief Defines the site size and textures
*/

void init_site()
{
   list = glGenLists(NLIST);

   glNewList(list, GL_COMPILE);
   for (int v = 0; v< (NUM_PANELS_FLOOR*4);)
   {
      bool text = (v < 4);
      // unbind the texture
      //glDisable(GL_TEXTURE_2D); //doesn't work
      if (v == 4)
      {
         // make the sides of the pool the fog color
         glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, fogColor);
         glColor3f ( fogColor[0], fogColor[1],  fogColor[2]) ;
      }
      else if (text)
      {
         glEnable(GL_TEXTURE_2D);
         glBindTexture(GL_TEXTURE_2D, texName[0]);
      }

      glBegin(GL_QUADS);
      if (text) glTexCoord2f(0.0, 0.0);
      glVertex3f(vertices[v].x, vertices[v].y, vertices[v].z);
      v++;
      if (text) glTexCoord2f(1.0, 0.0);
      glVertex3f(vertices[v].x, vertices[v].y, vertices[v].z);
      v++;
      if (text) glTexCoord2f(1.0, 1.0);
      glVertex3f(vertices[v].x, vertices[v].y, vertices[v].z);
      v++;
      if (text) glTexCoord2f(0.0, 1.0);
      glVertex3f(vertices[v].x, vertices[v].y, vertices[v].z);
      v++;
      glEnd();
      if (text) glDisable(GL_TEXTURE_2D);

   }

   glEndList();


   glNewList(list+1, GL_COMPILE);

   // material reacts to light
   const float orange_boards[] = { 0.5f, 0.25f, 0.0f };
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, orange_boards);
   glColor3f (orange_boards[0], orange_boards[1], orange_boards[2]) ;

   for (int i = 0; i< NUM_BOARDS; i++)
   {
      glTranslatef(b_positions[i].x, b_positions[i].y, b_positions[i].z);
      glRotatef(b_rotations[i], 0.0, 1.0, 0.0);

      for (int v = 0; v< NUM_PANELS_BOARDS; v++)
      {
         glBegin(GL_QUADS);
         glNormal3f(0.0,1.0,0.0);
         //glTexCoord2f(0.0, 0.0);
         glVertex3f(boards[v].x, boards[v].y, boards[v].z);
         v++;
         glNormal3f(0.0,1.0,0.0);
         //glTexCoord2f(1.0, 0.0);
         glVertex3f(boards[v].x, boards[v].y, boards[v].z);
         v++;
         glNormal3f(0.0,1.0,0.0);
         //glTexCoord2f(1.0, 1.0);
         glVertex3f(boards[v].x, boards[v].y, boards[v].z);
         v++;
         glNormal3f(0.0,1.0,0.0);
         //glTexCoord2f(0.0, 1.0);
         glVertex3f(boards[v].x, boards[v].y, boards[v].z);
         glEnd();
      }

      // undo rotation/translation
      glRotatef(-b_rotations[i], 0.0, 1.0, 0.0);
      glTranslatef(-b_positions[i].x, -b_positions[i].y, -b_positions[i].z);
   }

   glEndList();

   glNewList(list+2, GL_COMPILE);
   for (int i = 0; i< NUM_BOXES; i++)
   {
      glTranslatef(bx_positions[i].x, bx_positions[i].y, bx_positions[i].z);
      glRotatef(bx_rotations[i], 0.0, 1.0, 0.0);

      for (int v = 0; v< NUM_PANELS_BOXES; v++)
      {
         int k = 4*v;

         if (v == 0)
         {
            float mcolor[] = { 1.f, 1.f, 1.f, 1.f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mcolor);
            glColor3f ( mcolor[0], mcolor[1], mcolor[2]) ;
         }
         else if (v == (NUM_PANELS_BOXES-1))
         {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, texName[4 + i]);
            float mcolor[] = { 0.f, 0., 0.f, 1.f };
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mcolor);
            glColor3f (mcolor[0], mcolor[1], mcolor[2]);
         }

         glBegin(GL_QUADS);
         if (v == (NUM_PANELS_BOXES-1))
         {
            glNormal3f(0.0f,1.0f,0.0f);
            glTexCoord2f(0.0, 1.0);
         }
         glVertex3f(boxes[k].x, boxes[k].y, boxes[k].z);
         k++;
         if (v == (NUM_PANELS_BOXES-1))
         {
            glNormal3f(0.0f,1.0f,0.0f);
            glTexCoord2f(1.0, 1.0);
         }
         glVertex3f(boxes[k].x, boxes[k].y, boxes[k].z);
         k++;
         if (v == (NUM_PANELS_BOXES-1))
         {
            glNormal3f(0.0f,1.0f,0.0f);
            glTexCoord2f(1.0, 0.0);
         }
         glVertex3f(boxes[k].x, boxes[k].y, boxes[k].z);
         k++;
         if (v == (NUM_PANELS_BOXES-1))
         {
            glNormal3f(0.0f,1.0f,0.0f);
            glTexCoord2f(0.0, 0.0);
         }
         glVertex3f(boxes[k].x, boxes[k].y, boxes[k].z);
         glEnd();
         if (v == (NUM_PANELS_BOXES-1))
            glDisable(GL_TEXTURE_2D);
      }

      // undo rotation/translation
      glRotatef(-bx_rotations[i], 0.0, 1.0, 0.0);
      glTranslatef(-bx_positions[i].x, -bx_positions[i].y, -bx_positions[i].z);
   }
   glEndList();

   glNewList(list+3, GL_COMPILE);
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texName[1]);
   int v = 0;
   glBegin(GL_QUADS);

   glTexCoord2f(0.0, 0.0);
   glVertex3f(ceiling[v].x, ceiling[v].y, ceiling[v].z);
   v++;
   glTexCoord2f(4.0, 0.0);
   glVertex3f(ceiling[v].x, ceiling[v].y, ceiling[v].z);
   v++;
   glTexCoord2f(4.0, 4.0);
   glVertex3f(ceiling[v].x, ceiling[v].y, ceiling[v].z);
   v++;
   glTexCoord2f(0.0, 4.0);
   glVertex3f(ceiling[v].x, ceiling[v].y, ceiling[v].z);
   v++;
   glEnd();
   glDisable(GL_TEXTURE_2D);
   glEndList();

   /***********************************************************/

   g1 = gluNewQuadric();
   g2 = gluNewQuadric();
   g3 = gluNewQuadric();
   for (int i=0; i<WINDOW_CUTOUT_PARTS; i++)
      window_cutout[i] = gluNewQuadric();
   for (int i=0; i<GOALPOST_PARTS * 2; i++)
      goalpost[i] = gluNewQuadric();
   for (int i=0; i<NUM_BUOYS * 2; i++)
   {
      buoys[i] = gluNewQuadric();
      gluQuadricDrawStyle(buoys[i], GLU_FILL);
   }

   for (int i=0; i<2 * EXIT_SIDES; i++)
   {
      EXIT[i] = gluNewQuadric();
      gluQuadricDrawStyle(EXIT[i], GLU_FILL);
   }

   gluQuadricDrawStyle(g1,   GLU_FILL);
   gluQuadricDrawStyle(g2,   GLU_FILL);
   gluQuadricDrawStyle(g3,   GLU_FILL);
}

#define RADIUS 0.04
#define GHEIGHT 1.2
#define G1_X -5
#define G2_X -2
#define G1_Y -5

const float post_col[4] = {0.2,0.2,0.2,1};
const float red_bulb[4] = {1,0,0,1};

/** STARTING GATE POS AND COLOR */
void do_posts()
{
   /* gate */
   glColor3f ( 0.3f, 0.3f, 0.3f ) ;
   //  glNewList(list+3, GL_COMPILE);
   glRotatef(-90, 1.0, 0.0, 0.0);

   // horizontal bar
   glTranslatef(G2_X, G1_Y, POOL_HEIGHT);

   // random rotation and translation
   glRotatef(randNum % 50, 0.0, 0.0, -1.0);
   glTranslatef((randNum % 2353) / 1000.0, (randNum % 1941) / -2000.0, (randNum % 1761) / -3000.0);

   glRotatef(-90, 0.0, 1.0, 0.0);
   gluCylinder(g3,
               /*BASE_RADIUS*/ RADIUS,
               /*TOP_RADIUS*/ RADIUS,
               /*HEIGHT*/ -(G1_X-G2_X),
               /*SLICES*/ 10,
               /*STACKS*/ 10);
   glRotatef(90, 0.0, 1.0, 0.0);

   glColor3f ( 1.0f, 0.35f, 0.1f ) ;
   // axes are rotated, careful!!!
   glTranslatef(G1_X-G2_X, 0, -GHEIGHT);
   gluCylinder(g1,
               /*BASE_RADIUS*/ RADIUS,
               /*TOP_RADIUS*/ RADIUS,
               /*HEIGHT*/ GHEIGHT,
               /*SLICES*/ 10,
               /*STACKS*/ 10);

   glTranslatef(G2_X-G1_X, 0, 0);
   gluCylinder(g2,
               /*BASE_RADIUS*/ RADIUS,
               /*TOP_RADIUS*/ RADIUS,
               /*HEIGHT*/ GHEIGHT,
               /*SLICES*/ 10,
               /*STACKS*/ 10);

   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, post_col);
}

#define WINDOW_CUTOUT_X 10
#define WINDOW_CUTOUT_Y 2
#define WINDOW_CUTOUT_Z -1
#define WINDOW_CUTOUT_LENGTH 0.61
#define WINDOW_CUTOUT_RADIUS 0.015

/**
* @brief Define window cutout test obstacle
*/
void do_window_cutout()
{
   // base
   glColor3f( 0.0f, 0.0, 0.0f );
   glTranslatef(WINDOW_CUTOUT_X, 0.0, WINDOW_CUTOUT_Z);

   // random rotation
   glRotatef(60 + randNum % 80 - 40, 0.0, 1.0, 0.0);

   glRotatef(-90, 1.0, 0.0, 0.0);
   gluCylinder(window_cutout[0],
               /*BASE_RADIUS*/ WINDOW_CUTOUT_RADIUS,
               /*TOP_RADIUS*/ WINDOW_CUTOUT_RADIUS,
               /*HEIGHT*/ WINDOW_CUTOUT_Y,
               /*SLICES*/ 10,
               /*STACKS*/ 10);

   glRotatef(90, 1.0, 0.0, 0.0);
   glTranslatef(0, WINDOW_CUTOUT_Y, 0);

   // red window cutout
   glEnable(GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texName[8]);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0, 0.0);
   glVertex2f(WINDOW_CUTOUT_LENGTH/2, 0.0);
   glTexCoord2f(1.0, 0.0);
   glVertex2f(WINDOW_CUTOUT_LENGTH/2, WINDOW_CUTOUT_LENGTH);
   glTexCoord2f(1.0, 1.0);
   glVertex2f(-WINDOW_CUTOUT_LENGTH/2, WINDOW_CUTOUT_LENGTH);
   glTexCoord2f(0.0, 1.0);
   glVertex2f(-WINDOW_CUTOUT_LENGTH/2, 0.0);
   glEnd();

   glTranslatef(0.0, 0.0, 0.01);

   // blue window cutout
   glBindTexture(GL_TEXTURE_2D, texName[9]);
   glBegin(GL_QUADS);
   glTexCoord2f(0.0, 0.0);
   glVertex2f(WINDOW_CUTOUT_LENGTH/2, 0.0);
   glTexCoord2f(1.0, 0.0);
   glVertex2f(WINDOW_CUTOUT_LENGTH/2, WINDOW_CUTOUT_LENGTH);
   glTexCoord2f(1.0, 1.0);
   glVertex2f(-WINDOW_CUTOUT_LENGTH/2, WINDOW_CUTOUT_LENGTH);
   glTexCoord2f(0.0, 1.0);
   glVertex2f(-WINDOW_CUTOUT_LENGTH/2, 0.0);
   glEnd();

   glDisable(GL_TEXTURE_2D);
}

/**
* @brief Define red buoy obstacle (sim)
*/

const double BUOY_COLOR_PERIOD[] = {1.9, 2.2, 2.8};
const bool BUOY_CYLINDRICAL[] = {false, true, true};
void do_buoys()
{
   unsigned long long T = gettimeofday() / 1000000; // seconds
   
   for (unsigned i=0; i<NUM_BUOYS; i++)
   {
#ifdef CYCLE_COLORS 
      // small t is number of periods of time BUOY_COLOR_PERIOD[i]
      unsigned long long t = (unsigned long long) ((double)T / BUOY_COLOR_PERIOD[i]);
      if (i == 0) // cycle color on each period
          glColor3f ( 1.0f, 0.0f, 0.0f );
      else{
          if (t % 3 == (i-1) % 3)
              glColor3f ( 0.0f, 1.0f, 0.0f );
      	  else if (t % 3 == i % 3) 
              glColor3f ( 1.0f, 0.0f, 0.0f );
      	  else 
              glColor3f ( 1.0f, 1.0f, 0.0f );
      }
#else
      if (i == 0) // static colours
          glColor3f ( 0.0f, 1.0f, 0.0f );
      else if (i == 1) 
          glColor3f ( 1.0f, 0.0f, 0.0f );
      else 
          glColor3f ( 1.0f, 1.0f, 0.0f);

#endif
      glTranslatef(buoys_v[i].x, buoys_v[i].y, buoys_v[i].z);
      glRotatef(90, 1.0, 0.0, 0.0);

      if (BUOY_CYLINDRICAL[i]) {
        glTranslatef(0, 0, -0.25);
        gluCylinder(buoys[2 * i],
                  /*BASE_RADIUS*/ .05,//double size
                  /*TOP_RADIUS*/ .05,
                  /*HEIGHT*/ .25,
                  /*SLICES*/ 30,
                  /*STACKS*/ 5);
      }
      else {
        glScalef(1.0, 1.0, 1.4);
        gluSphere(buoys[2 * i],
                /* radius*/ .12,//double size
                /* slices*/ 40,
                /* stacks*/ 40);
      }
      // supports
      // RZ - remove for now
      glColor3f ( 0.0f, 0.0f, 0.0f );
      //gluCylinder(buoys[2 * i + 1],
      //            /*BASE_RADIUS*/ .01,
      //            /*TOP_RADIUS*/ .01,
      //            /*HEIGHT*/ buoys_v[i].y,
      //            /*SLICES*/ 10,
      //            /*STACKS*/ 10);
      glRotatef(-90, 1.0, 0.0, 0.0);
      glTranslatef(-buoys_v[i].x, -buoys_v[i].y, -buoys_v[i].z);
   }

}

#define GOALPOST_HEIGHT 1.2
#define GOALPOST_WIDTH 1.8
#define GOALPOST_X 5.20
#define GOALPOST_Y 2
#define GOALPOST_Z -6.45
#define GOALPOST_RADIUS 0.025
#define VERT_SPACE 0.2
#define VERT_FILL 0.3
/**
* @brief Define circumnavigating goalpost obstacle
*/
void do_goalpost()
{

   glColor3f (0.0f, 1.0f, 0.0f);
   glTranslatef(GOALPOST_X, GOALPOST_Y, GOALPOST_Z);
   gluCylinder(goalpost[0],
               /*BASE_RADIUS*/ GOALPOST_RADIUS,
               /*TOP_RADIUS*/ GOALPOST_RADIUS,
               /*HEIGHT*/ GOALPOST_WIDTH,
               /*SLICES*/ 10,
               /*STACKS*/ 10);

   // far side vertical piece
   glTranslatef (0, VERT_SPACE, 0); // up
   glRotatef(-90, 1.0, 0.0, 0.0); // pointing up
   gluCylinder(goalpost[1],
               /*BASE_RADIUS*/ GOALPOST_RADIUS,
               /*TOP_RADIUS*/ GOALPOST_RADIUS,
               /*HEIGHT*/ VERT_FILL,
               /*SLICES*/ 10,
               /*STACKS*/ 10);
   
   glColor3f (0.0f, 1.0f, 0.0f);
   // near side vertical piece
   glTranslatef (0, -GOALPOST_WIDTH, 0);
   gluCylinder(goalpost[2],
                /*BASE_RADIUS*/ GOALPOST_RADIUS,
                /*TOP_RADIUS*/ GOALPOST_RADIUS,
                /*HEIGHT*/ VERT_FILL,
                /*SLICES*/ 10,
                /*STACKS*/ 10);
    
   glColor3f (1.0f, 0.0f, 0.0f);
   // near side vertical piece
   glTranslatef (0, GOALPOST_WIDTH/2, 0);
   gluCylinder(goalpost[3],
                /*BASE_RADIUS*/ GOALPOST_RADIUS,
                /*TOP_RADIUS*/ GOALPOST_RADIUS,
                /*HEIGHT*/ GOALPOST_HEIGHT,
                /*SLICES*/ 10,
                /*STACKS*/ 10);

}

#define EXIT_radius 0.0254
#define SIDELENGTH 1.524 // 5 feet
#define DIAGLENGTH 0.8621 // sqrt(8) feet
#define TWOFEET 0.6096 // 2 feet
#define EXITLENGTH  2.743 // 9 feet

/**
* @brief Define both exit obstacles
*/
void do_exit(double x, double z, int offset) // draw exit
{
   glColor3f (0.0f, 0.0f, 1.0f);
   glTranslatef(Exit_v.x, Exit_v.y, Exit_v.z);
   glTranslatef(x, 0.0f, z);

   // sides
   gluCylinder(EXIT[0 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ SIDELENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(90.0, 0.0, 1.0, 0.0);
   glTranslatef(TWOFEET, 0, TWOFEET);

   gluCylinder(EXIT[1 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ SIDELENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glTranslatef(-EXITLENGTH, 0, 0);

   gluCylinder(EXIT[2 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ SIDELENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(90.0, 0.0, 1.0, 0.0);
   glTranslatef(-EXITLENGTH + TWOFEET, 0, TWOFEET);

   gluCylinder(EXIT[3 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ SIDELENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(135.0, 0.0, 1.0, 0.0);

   // diagonal sides

   gluCylinder(EXIT[4 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ DIAGLENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(-135.0, 0.0, 1.0, 0.0);
   glTranslatef(EXITLENGTH - TWOFEET, 0, -TWOFEET);
   glRotatef(45.0, 0.0, 1.0, 0.0);

   gluCylinder(EXIT[5 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ DIAGLENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(-45.0, 0.0, 1.0, 0.0);
   glTranslatef(0, 0, EXITLENGTH);
   glRotatef(135.0, 0.0, 1.0, 0.0);

   gluCylinder(EXIT[6 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ DIAGLENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);

   glRotatef(-135.0, 0.0, 1.0, 0.0);
   glTranslatef(-EXITLENGTH + TWOFEET * 2, 0, 0);
   glRotatef(225.0, 0.0, 1.0, 0.0);

   gluCylinder(EXIT[7 + offset],
               /*BASE_RADIUS*/ EXIT_radius,
               /*TOP_RADIUS*/ EXIT_radius,
               /*HEIGHT*/ DIAGLENGTH,
               /*SLICES*/ 3,
               /*STACKS*/ 3);
}

/**
* @brief Draw defined objects
*/
void draw()
{
   glCallList(list);

   // this is for lights turned off
   //glColor3f(1.0, 0.0, 0.0);

   // boards
   glCallList(list+1);



#if FOG_PANEL
   /* restore z buffer writes, set blend params & disable texturing */
   glEnable(GL_BLEND);
   // alpha can't be one (1-alpha=0)
   glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
   //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

   /* restore rendering state */
   glDisable(GL_BLEND);

#endif

   // boxes
   glCallList(list+2);

   // posts
   glPushMatrix();
   do_posts();
   glPopMatrix();

   glPushMatrix();
   do_buoys();
   glPopMatrix();

   glPushMatrix();
   do_window_cutout();
   glPopMatrix();

   glPushMatrix();
   do_goalpost();
   glPopMatrix();

#define EXIT_OFFSET_X -2.34f
#define EXIT_OFFSET_Z 3.36f

   glPushMatrix();
   do_exit(0.0f, 0.0f, 0);
   glPopMatrix();

   glPushMatrix();
   do_exit(EXIT_OFFSET_X, EXIT_OFFSET_Z, EXIT_SIDES);
   glPopMatrix();

   const float clear[] = { 0.f, 0.f, 0.0f, 1.0f };
   glColor3f (.527343, .804687, 1 ) ;
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, clear);
   glCallList(list+3);

   /*
     glEnable(GL_BLEND);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE);
     glutSolidCube(10);
     glDisable(GL_BLEND);
   */
}

/**
* @brief Destroy defined objects
*/
void destroy_site()
{
   glDeleteLists(list, NLIST);
}
