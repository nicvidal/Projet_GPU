
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "sotl.h"
#include "atom.h"
#include "vbo.h"
#include "ocl.h"
#include "shaders.h"
#include "default_defines.h"
#include "window.h"

#ifdef _SPHERE_MODE_
// SEGMENTS must be even
#define SEGMENTS 14
#define VERTICES (SEGMENTS*(SEGMENTS+1)+2)
#else
#define SEGMENTS 1
#define VERTICES 1
#endif

GLfloat *vbo_vertex;
GLfloat *vbo_color;
GLfloat *vbo_normal;
GLuint *tridx;          //approximation
GLuint nb_indexes = 0;
GLfloat *vertex_model;

GLuint nb_vertices = 0;
GLuint vertices_per_atom = 0;

static GLuint nbv = 0;
static GLuint vi = 0;
static GLuint ni = 0;
static GLuint ci = 0;

GLuint vbovid, vbocid;
#ifdef _SPHERE_MODE_
static GLuint vbonid, vboidx;
#endif

atom_skin_t atom_skin = SPHERE_SKIN;


sotl_color_t atom_color[2] = {
  //{0.0, 0.98, 0.98},           // Cyan
    {102.0/255, 204.0/255, 1.0}, // Blue, try ciel 102 204 255
  //{1.0, 102.0/255, 102.0/255}, // Red, try salmon : 255 102 102
  //{1.0, 116.0/255, 38.0/255},  // Orange
    {174.0/255, 74.0/255, 1.0},  // Purple
  //{0.95, 0.95, 0.0},           // Yellow
  //{0.0, 1.0, 0.78},            // Green
};


void vbo_initialize (unsigned natoms)
{
  vertices_per_atom = VERTICES;
  nb_vertices = VERTICES*natoms;

  vbo_vertex = xmalloc(3*nb_vertices*sizeof(float));
  vbo_color = xmalloc(3*nb_vertices*sizeof(GLfloat));
#ifdef _SPHERE_MODE_
  vertex_model = xmalloc(2*3*vertices_per_atom*sizeof(GLfloat));
  vbo_normal = xmalloc(3*nb_vertices*sizeof(GLfloat));
  tridx = xmalloc(2*3*nb_vertices*sizeof(GLuint));
#endif

  vbo_clear ();

#ifndef _SPHERE_MODE_
  shadersInit ();
#endif
}

void vbo_finalize (void)
{
    free(vbo_vertex);
    free(vbo_color);
#ifdef _SPHERE_MODE_
    free(vertex_model);
    free(vbo_normal);
    free(tridx);

    glDeleteBuffers(1, &vbonid);
    glDeleteBuffers(1, &vboidx);
#endif
    glDeleteBuffers(1, &vbocid);
    glDeleteBuffers(1, &vbovid);
}

void vbo_set_atoms_skin (atom_skin_t skin)
{
    atom_skin = skin;
}

void vbo_clear (void)
{
#ifdef _SPHERE_MODE_
    nbv = 0;
    nb_indexes = 0;
    vi = 0;
    ni = 0;
    ci = 0;
    vbo_add_atom (0.0, 0.0, 0.0); // Pacman model
    vbo_add_atom (0.0, 0.0, 0.0); // Ghost model

    memcpy(vertex_model, vbo_vertex, 2*3*vertices_per_atom*sizeof(GLfloat));
#endif
    nbv = 0;
    nb_indexes = 0;
    vi = 0;
    ni = 0;
    ci = 0;
}

void vbo_build (sotl_device_t *dev)
{
#ifdef _SPHERE_MODE_
    glGenBuffers(1, &vbonid);
    glBindBuffer(GL_ARRAY_BUFFER, vbonid);
    glNormalPointer(GL_FLOAT, 3*sizeof(float), 0);
    glBufferData(GL_ARRAY_BUFFER, nb_vertices*3*sizeof(float), vbo_normal, GL_STATIC_DRAW);
    dev->mem_allocated += nb_vertices*3*sizeof(float);
#endif

    glGenBuffers(1, &vbocid);
    glBindBuffer(GL_ARRAY_BUFFER, vbocid);
    glColorPointer(3, GL_FLOAT, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, nb_vertices*3*sizeof(float), vbo_color, GL_STATIC_DRAW);
    dev->mem_allocated += nb_vertices*3*sizeof(float);

    glGenBuffers(1, &vbovid);
    glBindBuffer(GL_ARRAY_BUFFER, vbovid);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glBufferData(GL_ARRAY_BUFFER, nb_vertices*3*sizeof(float), vbo_vertex, GL_STATIC_DRAW);
    dev->mem_allocated += nb_vertices*3*sizeof(float);

#ifdef _SPHERE_MODE_
    glGenBuffers(1, &vboidx);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboidx);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nb_indexes*sizeof(GLuint), tridx, GL_DYNAMIC_DRAW);
    dev->mem_allocated += nb_indexes*sizeof(GLuint);
#endif

#ifdef _SPHERE_MODE_
    glEnableClientState(GL_NORMAL_ARRAY);
#endif
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
}

extern GLfloat scale_factor;
extern GLfloat zcut[3];

void vbo_render (sotl_device_t *dev)
{
#ifdef _SPHERE_MODE_
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glDrawElements(GL_TRIANGLES, nb_indexes, GL_UNSIGNED_INT, 0);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

#else

    glUseProgram(shader_program);

    // Initialize PointSize of atoms (on the near clip)
    glUniform1f(psize_location, 2 * ATOM_RADIUS * scale_factor);

    // Pass min & max extents
    glUniform3f(minext_location, get_global_domain()->min_ext[0], get_global_domain()->min_ext[1], get_global_domain()->min_ext[2]);
    glUniform3f(maxext_location, get_global_domain()->max_ext[0], get_global_domain()->max_ext[1], get_global_domain()->max_ext[2]);

    if (dev->compute != SOTL_COMPUTE_OCL) {
      // Refresh vertices
      glBindBuffer(GL_ARRAY_BUFFER, vbovid);
      glBufferSubData(GL_ARRAY_BUFFER, 0, nb_vertices*3*sizeof(float), vbo_vertex);
      // Refresh colors
      glBindBuffer(GL_ARRAY_BUFFER, vbocid);
      glBufferSubData(GL_ARRAY_BUFFER, 0, nb_vertices*3*sizeof(float), vbo_color);      
    }

    glEnableClientState(GL_COLOR_ARRAY);

    glDrawArrays(GL_POINTS, 0, nb_vertices);

    glDisableClientState(GL_COLOR_ARRAY);

    glUseProgram(0);

#endif
}

#ifdef _SPHERE_MODE_
// Called after skin has been changed, so update vertices, triangles, normals and colors
void vbo_updateAtomCoordinatesToAccel(sotl_device_t *dev)
{

  // Update model buffer and execute update_vertices
  //
  ocl_updateModelFromHost(dev);

  // Update triangle buffer
  //
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboidx);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, nb_indexes*sizeof(GLuint), tridx);

  // Update color buffer
  //
  glBindBuffer(GL_ARRAY_BUFFER, vbocid);
  glBufferSubData(GL_ARRAY_BUFFER, 0, nb_vertices*3*sizeof(float), vbo_color);

  // Update normal buffer
  //
  glBindBuffer(GL_ARRAY_BUFFER, vbonid);
  glBufferSubData(GL_ARRAY_BUFFER, 0, nb_vertices*3*sizeof(float), vbo_normal);
}
#endif

#ifdef _SPHERE_MODE_
static void addVertice(GLfloat x, GLfloat y, GLfloat z, GLfloat nx, GLfloat ny, GLfloat nz)
{
    vbo_vertex[vi++] = x;
    vbo_vertex[vi++] = y;
    vbo_vertex[vi++] = z;
    //vbo_vertex[vi++] = 1.0f;

    vbo_normal[ni++] = nx;
    vbo_normal[ni++] = ny;
    vbo_normal[ni++] = nz;
    //  vbo_normal[ni++] = 1.0f;

    vbo_color[ci++] = atom_color[0].R;
    vbo_color[ci++] = atom_color[0].G;
    vbo_color[ci++] = atom_color[0].B;
    //  vbo_color[ci++] = 1.0f;
}
#else
static void addVertice(GLfloat x, GLfloat y, GLfloat z)
{
    vbo_vertex[vi++] = x;
    vbo_vertex[vi++] = y;
    vbo_vertex[vi++] = z;
    //vbo_vertex[vi++] = 1.0f;

    vbo_color[ci++] = atom_color[0].R;
    vbo_color[ci++] = atom_color[0].G;
    vbo_color[ci++] = atom_color[0].B;
    //  vbo_color[ci++] = 1.0f;
}
#endif

#ifdef _SPHERE_MODE_
static void addSphere(GLfloat cx, GLfloat cy, GLfloat cz, double radius)
{
    int i,j;
    float lon,lat;
    float loninc,latinc;
    float x,y,z;
    GLuint bl, br, tl, tr;

    loninc = 2*M_PI/(SEGMENTS);
    latinc = M_PI/SEGMENTS;

    bl = nbv;

    // Create SEGMENTS/2-1 fake points to match #vertices of ghosts...
    for(i=0; i<=SEGMENTS/2; i++) {
        addVertice(cx, cy-radius, cz, 0, -1, 0);
        nbv++;
    }

    lon = 0;
    lat = -M_PI/2 + latinc;
    y = sin(lat);
    for (i=0; i<=SEGMENTS-1; i++) {
        x = cos(lon)*cos(lat);
        z = -sin(lon)*cos(lat);

        addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

        if(i == 0)
            tl = nbv;
        else {
            tr = nbv;
            tridx[nb_indexes++] = bl;
            tridx[nb_indexes++] = tl;
            tridx[nb_indexes++] = tr;
            tl = tr;
        }

        nbv++;
        lon += loninc;
    }

    tridx[nb_indexes++] = bl;
    tridx[nb_indexes++] = tl;
    tridx[nb_indexes++] = nbv-SEGMENTS;

    for (j=1; j<SEGMENTS-1; j++) { 
        lon = 0;
        lat += latinc;
        for (i=0; i<SEGMENTS; i++) {

            x = cos(lon)*cos(lat);
            y = sin(lat);
            z = -sin(lon)*cos(lat);

            addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

            if(i == 0) {
                bl = nbv-SEGMENTS;
                tl = nbv;
            } else {
                tr = nbv;
                br = bl+1;
                tridx[nb_indexes++] = bl;
                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = br;

                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = tr;
                tridx[nb_indexes++] = br;

                tl++;
                bl++;
            }

            nbv++;
            lon += loninc;
        }

        tr = nbv - SEGMENTS;
        br = tr - SEGMENTS;

        tridx[nb_indexes++] = bl;
        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = br;

        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = tr;
        tridx[nb_indexes++] = br;

        // Duplicate Equator (for the eating effect)
        if(j == SEGMENTS/2-1) {
            lon = 0;
            for (i=0; i<SEGMENTS; i++) {
                x = cos(lon)*cos(lat);
                y = sin(lat);
                z = -sin(lon)*cos(lat);

                addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);
                nbv++;

                lon += loninc;
            }
        }
    }

    tl = nbv;

    for (i=0; i<SEGMENTS; i++) {

        if(i == 0)
            bl = nbv - SEGMENTS;
        else {
            br = bl+1;
            tridx[nb_indexes++] = tl;
            tridx[nb_indexes++] = bl;
            tridx[nb_indexes++] = br;
            bl = br;
        }
    }

    tridx[nb_indexes++] = tl;
    tridx[nb_indexes++] = bl;
    tridx[nb_indexes++] = nbv-SEGMENTS;

    // Create SEGMENTS/2-1 fake points to match #vertices of ghosts...
    for(i=0; i<=SEGMENTS/2; i++) {
        addVertice(cx, cy+radius, cz, 0, 1, 0);
        nbv++;
    }
}

static void addGhost(GLfloat cx, GLfloat cy, GLfloat cz, double radius)
{
    int i,j;
    float lon,lat;
    float loninc,latinc;
    float x,y,z;
    GLuint bl, br, tl, tr;

    loninc = 2*M_PI/(SEGMENTS);
    latinc = M_PI/SEGMENTS;

    tl = nbv;

    addVertice(cx, cy+radius, cz, 0, 1, 0);
    nbv++;

    lon = 0;
    lat = M_PI/2 - latinc;
    y = sin(lat);
    for (i=0; i<SEGMENTS; i++) {
        x = cos(lon)*cos(lat);
        z = -sin(lon)*cos(lat);

        addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

        if(i == 0)
            bl = nbv;
        else {
            br = nbv;
            tridx[nb_indexes++] = tl;
            tridx[nb_indexes++] = bl;
            tridx[nb_indexes++] = br;
            bl = br;
        }

        nbv++;
        lon += loninc;
    }

    tridx[nb_indexes++] = tl;
    tridx[nb_indexes++] = bl;
    tridx[nb_indexes++] = nbv-SEGMENTS;

    for (j=1; j<SEGMENTS/2; j++) { 
        lon = 0;
        lat -= latinc;
        for (i=0; i<SEGMENTS; i++) {

            x = cos(lon)*cos(lat);
            y = sin(lat);
            z = -sin(lon)*cos(lat);

            addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

            if(i == 0) {
                tl = nbv-SEGMENTS;
                bl = nbv;
            } else {
                br = nbv;
                tr = tl+1;
                tridx[nb_indexes++] = bl;
                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = br;

                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = tr;
                tridx[nb_indexes++] = br;

                tl++;
                bl++;
            }

            nbv++;
            lon += loninc;
        }

        br = nbv - SEGMENTS;
        tr = br - SEGMENTS;

        tridx[nb_indexes++] = bl;
        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = br;

        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = tr;
        tridx[nb_indexes++] = br;
    }

    // Duplicate equator
    lon = 0;
    for (i=0; i<SEGMENTS; i++) {
        x = cos(lon)*cos(lat);
        y = sin(lat);
        z = -sin(lon)*cos(lat);

        addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);
        nbv++;

        lon += loninc;
    }

    for (; j<SEGMENTS-1; j++) { 
        lon = 0;
        y -= 0.15;
        for (i=0; i<SEGMENTS; i++) {

            x = cos(lon)*cos(lat);
            z = -sin(lon)*cos(lat);

            addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

            if(i == 0) {
                tl = nbv-SEGMENTS;
                bl = nbv;
            } else {
                br = nbv;
                tr = tl+1;
                tridx[nb_indexes++] = bl;
                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = br;

                tridx[nb_indexes++] = tl;
                tridx[nb_indexes++] = tr;
                tridx[nb_indexes++] = br;

                tl++;
                bl++;
            }

            nbv++;
            lon += loninc;
        }

        br = nbv - SEGMENTS;
        tr = br - SEGMENTS;

        tridx[nb_indexes++] = bl;
        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = br;

        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = tr;
        tridx[nb_indexes++] = br;
    }

    lon = loninc/2;
    y -= 0.2;
    tl = nbv-SEGMENTS;
    for (i=0; i<SEGMENTS; i++) {

        x = cos(lon)*cos(lat);
        z = -sin(lon)*cos(lat);

        addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);

        bl = nbv;
        if(i == SEGMENTS-1)
            tr = tl -SEGMENTS+1;
        else
            tr = tl+1;
        tridx[nb_indexes++] = tl;
        tridx[nb_indexes++] = tr;
        tridx[nb_indexes++] = bl;
        tl++;

        nbv++;
        lon += loninc;
    }

    // Add a fake vertex
    addVertice(cx+x*radius, cy+y*radius, cz+z*radius, x, y, z);
    nbv++;
}
#endif

void vbo_add_atom(GLfloat cx, GLfloat cy, GLfloat cz)
{
#ifdef _SPHERE_MODE_
    static unsigned odd = 0;
    switch(atom_skin) {
        case PACMAN_SKIN :
            if(odd) {
                atom_color[0].R = 1.0;
                atom_color[0].G = 1.0;
                atom_color[0].B = 1.0;
                addGhost(cx, cy, cz, ATOM_RADIUS);
            } else {
                atom_color[0].R = 1.0;
                atom_color[0].G = 1.0;
                atom_color[0].B = 0.0;
                addSphere(cx, cy, cz, ATOM_RADIUS);
            }
            odd ^= 1;
            break;
        case SPHERE_SKIN :
        default :
            atom_color[0].R = 0.0;
            atom_color[0].G = 250.0/255.0;
            atom_color[0].B = 250.0/255.0;
            addSphere(cx, cy, cz, ATOM_RADIUS);
    }
#else
    addVertice(cx, cy, cz);
    nbv++;
#endif
}
