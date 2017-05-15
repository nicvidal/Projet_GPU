
#ifndef _VBO_H_IS_DEF_
#define _VBO_H_IS_DEF_

#ifdef __APPLE__
#include <OpenGL/gl.h>          // Header File For The OpenGL32 Library
#include <OpenGL/glu.h>         // Header File For The GLu32 Library
#include <GLUT/glut.h>          // Header File For The GLut Library
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>          // Header File For The OpenGL32 Library
#include <GL/glu.h>         // Header File For The GLu32 Library
#include <GL/glut.h>          // Header File For The GLut Library
#endif

#include "default_defines.h"
#include "device.h"

typedef enum { SPHERE_SKIN, PACMAN_SKIN } atom_skin_t;

extern GLfloat *vbo_vertex;
extern GLfloat *vbo_color;
extern GLfloat *vertex_model;

extern GLuint vbovid, vbocid;

extern GLuint nb_vertices;
extern GLuint vertices_per_atom;

typedef struct {
  GLfloat R, G, B;
} sotl_color_t;

extern sotl_color_t atom_color[];

void vbo_initialize (unsigned natoms);
void vbo_build (sotl_device_t *dev);
void vbo_set_atoms_skin (atom_skin_t skin);
void vbo_add_atom(GLfloat cx, GLfloat cy, GLfloat cz);
void vbo_clear (void);
void vbo_render (sotl_device_t *dev);
void vbo_updateAtomCoordinatesToAccel(sotl_device_t *dev);
void vbo_finalize (void);

#endif
