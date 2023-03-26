/* HEAVILY WIP */
#ifndef _GLASS_GL_H
#define _GLASS_GL_H

#include "glcommon.h"

// Color

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void glColorPointer(GLint size, GLenum type, GLsizei stride,
                    const void *pointer);

// Fog

void glFogf(GLenum pname, GLfloat param);

// Light

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void glLightf(GLenum light, GLenum pname, GLfloat param);
void glLightModelf(GLenum pname, GLfloat param);
void glLightModelfv(GLenum pname, const GLfloat *params);

// Material

void glMaterialf(GLenum face, GLenum pname, GLfloat param);
void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);

// Matrix

void glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
                GLfloat near, GLfloat far);
void glMatrixMode(GLenum mode);
void glMultMatrixf(const GLfloat *m);
void glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
              GLfloat near, GLfloat far);
void glPushMatrix(void);
void glPopMatrix(void);
void glLoadIdentity(void);
void glLoadMatrixf(const GLfloat *m);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);

// TexEnv

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
void glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
void glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void glTexEnvi(GLenum target, GLenum pname, GLint param);

/*
void glClientActiveTexture(GLenum texture); // NOT DONE
void glClipPlanef(GLenum plane,
                  const GLfloat *equation); // NOT DONE
void glDisableClientState(GLenum array);
void glEnableClientState(GLenum array);

void glGetClipPlanef(GLenum plane, GLfloat *equation);


void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void glGetPointerv(GLenum pname, void **params);

void glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r,
                       GLfloat q);

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);

void glNormalPointer(GLenum type, GLsizei stride, const void *pointer);

void glPointParameterf(GLenum pname, GLfloat param);

void glPointSize(GLfloat size);

void glPointSizePointerOES(GLenum type, GLsizei stride, const void *pointer);

void glShadeModel(GLenum mode);

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride,
                       const void *pointer);

void glVertexPointer(GLint size, GLenum type, GLsizei stride,
                     const void *pointer);
*/

#endif /* _GLASS_GL_H */