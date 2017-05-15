
#ifndef _SHADERS_H_IS_DEF_
#define _SHADERS_H_IS_DEF_

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

extern GLuint shader_program;
extern GLuint psize_location,
  minext_location,
  maxext_location;

void shadersInit(void);

#endif
