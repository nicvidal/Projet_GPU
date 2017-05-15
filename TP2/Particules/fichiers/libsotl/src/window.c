#define _XOPEN_SOURCE 600

#include <math.h>

#include "device.h"
#include "sotl.h"
#include "window.h"
#include "global_definitions.h"
#include "default_defines.h"
#include "atom.h"
#include "ocl.h"
#include "ocl_kernels.h"
#include "device.h"

#include "shaders.h"
#include "vbo.h"

float zcut[3]; // allows to distinguish up to 4 different computation zones

static unsigned max_iter = 0;

static unsigned long TIMER_VAL = 5;	/* atom position update interval (in
                                * milliseconds) */

static void (*glut_callback_func)(void) = NULL;


static GLuint glutWindowHandle = 0;
static GLdouble fovy, aspect, near_clip, far_clip;
GLfloat scale_factor;

/*
 * parameters for gluPerspective() 
 */

// mouse controls
static calc_t dis;

static calc_t eye[3];			/* position of eye point */
static calc_t center[3];		/* position of look reference point */
static calc_t up[3];			/* up direction for camera */

static int mouse_old_x, mouse_old_y;
static int mouse_buttons = 0;
static calc_t rotate_x = 0.0, rotate_y = 0.0;
static calc_t translate_z = 0.0;

static void reshape (int w, int h)
{
  glViewport (0, 0, (GLsizei) w, (GLsizei) h);
  aspect = w / (calc_t) h;
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (fovy, aspect, near_clip, far_clip);
  glMatrixMode (GL_MODELVIEW);
}

static unsigned true_redisplay = 1;

static float mvmatrix[4][4];
float normalized_vert[3] = { 0.0, 1.0, 0.0 };

static void drawScene ()
{

  if(sotl_rotate_camera) {
    float p, q;
    q = rotate_y + 180.0; q = q/360.0 * 2 * M_PI;
    rotate_y -= 0.15;
    while(rotate_y < 0.0)
      rotate_y += 360.0;

    p = rotate_y + 180.0; p = p/360.0 * 2 * M_PI;
    
    //rotate_x += 15.0 * sin((rotate_y + 180.0)/ 360.0 * 2 * 3.14159);
    rotate_x += 15.0 * (sin(p) - sin(q));

    true_redisplay = 1;
  }

  if (true_redisplay) {
    glLoadIdentity ();
    /*
     * Define viewing transformation 
     */
    gluLookAt ((GLdouble) eye[0], (GLdouble) eye[1], (GLdouble) eye[2],
	       (GLdouble) center[0], (GLdouble) center[1], (GLdouble) center[2], 
	       (GLdouble) up[0], (GLdouble) up[1], (GLdouble) up[2]);

    glTranslatef (center[0], center[1], center[2] + translate_z);
    glRotatef (rotate_x, 1.0, 0.0, 0.0);
    glRotatef (rotate_y, 0.0, 1.0, 0.0);
    glTranslatef (-center[0], -center[1], -center[2]);

    glGetFloatv(GL_MODELVIEW_MATRIX, &mvmatrix[0][0]);
    normalized_vert[0] = mvmatrix[0][1];
    normalized_vert[1] = mvmatrix[1][1];
    normalized_vert[2] = mvmatrix[2][1];

    true_redisplay = 0;
  }

  vbo_render (sotl_display_device ());

  // Draw a nice floor :)
  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin (GL_TRIANGLE_FAN);
  glColor4f (.6f, 0.6f, 1.0f, 0.4f);
  glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->min_ext[1], get_global_domain()->min_ext[2]); glNormal3f(0.0, 1.0, 0.0);
  glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->min_ext[1], get_global_domain()->max_ext[2]); glNormal3f(0.0, 1.0, 0.0);
  glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->min_ext[1], get_global_domain()->max_ext[2]); glNormal3f(0.0, 1.0, 0.0);
  glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->min_ext[1], get_global_domain()->min_ext[2]); glNormal3f(0.0, 1.0, 0.0);
  glEnd ();

  // Draw normalized vertical
  /* glEnable (GL_BLEND); */
  /* glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */
  /* glBegin (GL_LINES); */
  /* glColor4f (1.f, 1.f, 0.f, 1.f); */
  /* glVertex3f (center[0], center[1], center[2]); */
  /* glVertex3f (center[0] + normalized_vert[0], */
  /* 	      center[1] + normalized_vert[1], */
  /* 	      center[2] + normalized_vert[2]); */
  /* glEnd (); */
  

#ifdef SHOW_GRID
  // Draw partitionning wire frames

  /* for(calc_t z = get_global_domain()->min_ext[2]; z <= get_global_domain()->max_ext[2]; z += BOX_SIZE) { */
  /*   glBegin (GL_LINE_LOOP); */

  /*   if(z < zcut[1]) */
  /* 	glColor4f (atom_color[0].R, atom_color[0].G, atom_color[0].B, 1.0f); */
  /*   else */
  /* 	glColor4f (atom_color[1].R, atom_color[1].G, atom_color[1].B, 1.0f); */
	
  /*   glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->min_ext[1], z); */
  /*   glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->min_ext[1], z); */
  /*   glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->max_ext[1], z); */
  /*   glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->max_ext[1], z); */

  /*   glEnd (); */
  /* } */

  for(unsigned i = 0; i < 3 ; i++) {
    glBegin (GL_LINE_LOOP);
    switch(i) {
    case 0 : 
      glColor4f (atom_color[0].R, atom_color[0].G, atom_color[0].B, 1.0f);
      break;
    case 1 :
      glColor4f (1.0f, 1.0f, 1.0f, 0.9f);
      break;
    case 2 :
      glColor4f (atom_color[1].R, atom_color[1].G, atom_color[1].B, 1.0f);
      break;
    }
    glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->min_ext[1], zcut[i]);
    glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->min_ext[1], zcut[i]);
    glVertex3f (get_global_domain()->max_ext[0], get_global_domain()->max_ext[1], zcut[i]);
    glVertex3f (get_global_domain()->min_ext[0], get_global_domain()->max_ext[1], zcut[i]);
    glEnd ();
  }
#endif

}

static void display (void)
{
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawScene ();
    glutSwapBuffers ();
}

static void appDestroy ()
{
  sotl_finalize ();

  if (glutWindowHandle)
    glutDestroyWindow (glutWindowHandle);

  exit (0);
}


static void timer (int arg)
{
  if(max_iter != 0 && force_enabled) {
    static unsigned i = 0;

    if (++i > max_iter) {
      if (sotl_verbose)
	sotl_log(INFO, "Stopping after %d iterations\n", max_iter);
      appDestroy ();
    }
  }

  glut_callback_func ();

  glutPostRedisplay();

  glutTimerFunc(TIMER_VAL, timer, arg);
}

static void idle (void)
{
  if (max_iter != 0 && force_enabled) {
    static unsigned i = 0;

    if (++i > max_iter) {
      if (sotl_verbose)
	sotl_log(INFO, "Stopping after %d iterations\n", max_iter);
      appDestroy ();
    }
  }

  glut_callback_func ();

  glutPostRedisplay ();
}

static void initView (unsigned winx, unsigned winy)
{
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { 0.5, 0.5, 1.0, 0.0 };
  calc_t dif_ext[3];
  int i;
  GLfloat near;

  // Define normal light 
  glLightfv (GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv (GL_LIGHT0, GL_POSITION, light_position);

  // Enable a single OpenGL light 
  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);

  // Use depth buffering for hidden surface elimination 
  glEnable (GL_DEPTH_TEST);

  glEnable (GL_POINT_SPRITE);
  glEnable (GL_POINT_SMOOTH);
  glEnable (GL_VERTEX_PROGRAM_POINT_SIZE);

  // get diagonal and average distance of extent 
  for (i = 0; i < 3; i++)
    dif_ext[i] = get_global_domain()->max_border[i] - get_global_domain()->min_border[i];

  dis = 0.0;
  for (i = 0; i < 3; i++)
    //dis += dif_ext[i] * dif_ext[i];
    dis = MAX(dis, dif_ext[i]);
  //dis = (calc_t) sqrt ((double) dis);

  dis *= 1.2;

  // set center in world space 
  for (i = 0; i < 3; i++)
    center[i] = get_global_domain()->min_border[i] + dif_ext[i] * 0.5;

  // set initial eye & look at location in world space 
  eye[0] = center[0];
  eye[1] = center[1];
  eye[2] = center[2] + dis;
  up[0] = 0.0;
  up[1] = 1.0;
  up[2] = 0.0;

  // Near- & far clip-plane distances 
  near = dis - 0.5 * dif_ext[2];
  //near_clip = (GLdouble) (0.5 * (dis - 0.5 * dif_ext[2]));
  near_clip = (GLdouble) (0.5 * near);
  far_clip = (GLdouble) (2.0 * (dis + 0.5 * dif_ext[2]));

  // Field of view 
  fovy = (GLdouble) (0.5 * dif_ext[1] / near);
  fovy = (GLdouble) (2 * atan ((double) fovy) / M_PI * 180.0);

  {
    GLfloat scalex = (float)winx / (get_global_domain()->max_border[0]-get_global_domain()->min_border[0]),
      scaley = (float)winy / (get_global_domain()->max_border[1]-get_global_domain()->min_border[1]);

    scale_factor = near * ((scalex < scaley) ? scalex : scaley);
  }

  // Enable the color material mode 
  glEnable (GL_COLOR_MATERIAL);
}

#ifdef _SPHERE_MODE_
void change_skin(atom_skin_t skin, sotl_atom_set_t *set)
{
  sotl_device_t *dev = sotl_display_device ();

  ocl_acquire(dev);

  // Set skin to Pacmans and ghosts
  vbo_set_atoms_skin (skin);

  // Reset all vertices & triangles indexes
  vbo_clear ();

  // Re-calculate colors, normals, and triangles
  atom_build (set->natoms, &set->pos);

  // Resfreh vertices in Accelerator memory
  vbo_updateAtomCoordinatesToAccel(dev);

  // Reset eating animation
  resetAnimation();

  ocl_release(dev);

  glutPostRedisplay();
}
#endif

static void appKeyboard (unsigned char key, int x, int y)
{
  // this way we can exit the program cleanly
  switch (key)
    {
    case '<':
      translate_z -= 0.1;
      true_redisplay = 1;
      glutPostRedisplay ();
      break;

    case '>':
      translate_z += 0.1;
      true_redisplay = 1;
      glutPostRedisplay ();
      break;

    case '+':
      TIMER_VAL = TIMER_VAL - TIMER_VAL / 10;
      break;

    case '-':
      TIMER_VAL = TIMER_VAL + (TIMER_VAL / 10 ? : 1);
      break;
#ifdef _SPHERE_MODE_
    case 'n' :
    case 'N' : 
      sotl_log(INFO, "atom mode\n");
      change_skin(SPHERE_SKIN, &sotl_display_device ()->atom_set); /* FIXME */
      break; // atom mode
    case 'e' :
    case 'E' : 
      eating_enabled = 1 - eating_enabled;
      sotl_log(INFO, "eating mode: %d\n", eating_enabled); break;
    case 'h' :
    case 'H' :
      growing_enabled = 1 - growing_enabled;
      sotl_log(INFO, "growing mode: %d\n", growing_enabled); break;
    case 'p' : 
    case 'P' : 
      sotl_log(INFO, "pacman mode\n");
      change_skin(PACMAN_SKIN, &sotl_display_device ()->atom_set); /* FIXME */
      break; // pacman mode
#endif
    case 'z':
    case 'Z':
      for (unsigned d = 0; d < sotl_nb_devices; ++d)
	zero_speed_kernel (sotl_devices[d]);
      break;

    case 'm':
    case 'M':
      move_enabled = 1 - move_enabled;
      sotl_log(INFO, "move: %d\n", move_enabled);
      break;

    case 'f':
    case 'F':
      force_enabled = 1 - force_enabled;
      sotl_log(INFO, "force: %d\n", force_enabled);
      break;

    case 'g':
    case 'G':
      gravity_enabled = 1 - gravity_enabled;
      sotl_log(INFO, "gravity: %d\n", gravity_enabled);
      break;

    case 'c':
    case 'C':
      detect_collision = 1 - detect_collision;
      sotl_log(INFO, "collision detect: %d\n", detect_collision);
      break;

    case 'b':
    case 'B':
      borders_enabled = 1 - borders_enabled;
      sotl_log(INFO, "borders: %d\n", borders_enabled);
      break;

    case 'r':
    case 'R':
      sotl_rotate_camera = 1 - sotl_rotate_camera;
      sotl_log(INFO, "dynamic camera: %d\n", sotl_rotate_camera);
      break;

    case '\033':		// escape quits
    case '\015':		// Enter quits 
    case 'Q':			// Q quits
    case 'q':			// q (or escape) quits
      // Cleanup up and quit
      appDestroy ();
      break;

    default:
      sotl_log(WARNING, "undefined key %c (%d, %d)\n", key, x, y);
      break;
    }
}

static void appMouse (int button, int state, int x, int y)
{
  // handle mouse interaction for rotating/zooming the view
  if (state == GLUT_DOWN) {
        mouse_buttons |= 1 << button;
  } else if (state == GLUT_UP) {
    mouse_buttons = 0;
  }

  mouse_old_x = x;
  mouse_old_y = y;
}

static void appMotion (int x, int y)
{
  // hanlde the mouse motion for zooming and rotating the view
  calc_t dx, dy;

  dx = x - mouse_old_x;
  dy = y - mouse_old_y;

  if (mouse_buttons & 1) {
    rotate_x += dy * 0.2;
    rotate_y += dx * 0.2;
  }

  mouse_old_x = x;
  mouse_old_y = y;

  true_redisplay = 1;
  glutPostRedisplay ();
}

void setZcutValues(float zcut0, float zcut1, float zcut2)
{
  zcut[0] = zcut0;
  zcut[1] = zcut1;
  zcut[2] = zcut2;
}

void window_main_loop(void (*callback)(void), unsigned nb_iter)
{
  glut_callback_func = callback;
  max_iter = nb_iter;

  glutMainLoop ();
}

void window_opengl_init (unsigned winx, unsigned winy,
			 calc_t min_ext[], calc_t max_ext[], int full_speed)
{
  int argc = 1;
  char *argv[] = { "foo", NULL };

  glutInit (&argc, argv);

  // Initialize display mode 
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

  glutInitWindowSize (winx, winy);

  glutWindowHandle = glutCreateWindow ("Lennard-Jones Atoms");
  
  initView (winx, winy);

  setZcutValues(min_ext[2], min_ext[2] + 0.5 * (max_ext[2] - min_ext[2]), max_ext[2]);

  glutDisplayFunc (display);
  glutReshapeFunc (reshape);
  if (full_speed)
    glutIdleFunc (idle);
  else
    glutTimerFunc(TIMER_VAL, timer, 0);

  glutKeyboardFunc (appKeyboard);
  glutMouseFunc (appMouse);
  glutMotionFunc (appMotion);
}
