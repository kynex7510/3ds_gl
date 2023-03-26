#ifndef _GLASS_GLCOMMON_H
#define _GLASS_GLCOMMON_H

#include "gldef.h"

// Buffers

void glBindBuffer(GLenum target, GLuint buffer);
void glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data,
                  GLenum usage);
void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                     const GLvoid *data);
void glDeleteBuffers(GLsizei n, const GLuint *buffers);
void glGenBuffers(GLsizei n, GLuint *buffers);
void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params);
GLboolean glIsBuffer(GLuint buffer);

// Effects

void glAlphaFunc(GLenum func, GLclampf ref);
void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glBlendEquation(GLenum mode);
void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
void glBlendFunc(GLenum sfactor, GLenum dfactor);
void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha,
                         GLenum dstAlpha);
void glColorMask(GLboolean red, GLboolean green, GLboolean blue,
                 GLboolean alpha);
void glCullFace(GLenum mode);
void glDepthFunc(GLenum func);
void glDepthMask(GLboolean flag);
void glDepthRangef(GLclampf nearVal, GLclampf farVal);
void glFrontFace(GLenum mode);
void glLogicOp(GLenum opcode);
void glPolygonOffset(GLfloat factor, GLfloat units);
void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void glStencilFunc(GLenum func, GLint ref, GLuint mask);
void glStencilMask(GLuint mask);
void glStencilOp(GLenum sfail, GLenum dpfail, GLenum dppass);
void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// Extensions

void glBlockModePICA(GLenum mode);
void glClearEarlyDepthPICA(GLclampf depth);
void glCombinerColorPICA(GLclampf red, GLclampf green, GLclampf blue,
                         GLclampf alpha);
void glCombinerFuncPICA(GLenum pname, GLenum func);
void glCombinerOpPICA(GLenum pname, GLenum op);
void glCombinerScalePICA(GLenum pname, GLfloat scale);
void glCombinerSrcPICA(GLenum pname, GLenum src);
void glCombinerStagePICA(GLint index);
void glEarlyDepthFuncPICA(GLenum func);
void glFragOpPICA(GLenum mode);

// Rendering

void glClear(GLbitfield mask);
void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepthf(GLclampf depth);
void glClearStencil(GLint s);
void glDrawArrays(GLenum mode, GLint first, GLsizei count);
void glDrawElements(GLenum mode, GLsizei count, GLenum type,
                    const GLvoid *indices);
void glFinish(void);
void glFlush(void);
void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *data);

// State

void glDisable(GLenum cap);
void glEnable(GLenum cap);
GLboolean glIsEnabled(GLenum cap);
GLenum glGetError(void);
void glGetBooleanv(GLenum pname, GLboolean *params);
void glGetFloatv(GLenum pname, GLfloat *params);
void glGetIntegerv(GLenum pname, GLint *params);
const GLubyte *glGetString(GLenum name);

// Texture

void glActiveTexture(GLenum texture);
void glBindTexture(GLenum target, GLuint texture);
void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLsizei imageSize, const GLvoid *data);
void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                               GLint yoffset, GLsizei width, GLsizei height,
                               GLenum format, GLsizei imageSize,
                               const GLvoid *data);
void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border);
void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                         GLint yoffset, GLint x, GLint y, GLsizei width,
                         GLsizei height);
void glDeleteTextures(GLsizei n, const GLuint *textures);
void glGenTextures(GLsizei n, GLuint *textures);
void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
GLboolean glIsTexture(GLuint texture);
void glTexImage2D(GLenum target, GLint level, GLint internalformat,
                  GLsizei width, GLsizei height, GLint border, GLenum format,
                  GLenum type, const GLvoid *data);
void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void glTexParameteri(GLenum target, GLenum pname, GLint param);
void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type,
                     const GLvoid *data);

#endif /* _GLASS_GLCOMMON_H */