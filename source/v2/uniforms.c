#include "context.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

#define CheckOffset(type, offset)                                              \
  ((((type) == GLASS_UNI_BOOL) && ((offset) < 16)) ||                          \
   (((type) == GLASS_UNI_INT) && ((offset) < 4)) ||                            \
   (((type) == GLASS_UNI_FLOAT) && ((offset) < 96)))

// Helpers

#define MakeLocation GLASS_uniforms_makeLocation
static GLint GLASS_uniforms_makeLocation(const size_t index,
                                         const size_t offset,
                                         const bool isGeometry) {
  return (GLint)((isGeometry << 16) | (index << 8) | (offset & 0xFF));
}

#define ExtractOffset GLASS_uniforms_extractOffset
static size_t GLASS_uniforms_extractOffset(const char *name) {
  if (strstr(name, ".") || (strstr(name, "gl_") == name))
    return -1;

  const char *beg = strstr(name, "[");
  if (!beg)
    return 0;

  const char *end = strstr(beg, "]");
  if (!end || end[1] != '\0' || !(end - beg))
    return -1;

  return atoi(&end[1]);
}

#define LookupUniform GLASS_uniforms_lookupUniform
static GLint GLASS_uniforms_lookupUniform(const ShaderInfo *shader,
                                          const char *name,
                                          const size_t offset) {
  for (size_t i = 0; i < shader->numOfActiveUniforms; i++) {
    const UniformInfo *uni = &shader->activeUniforms[i];
    if (strstr(name, uni->symbol) == name) {
      if (!CheckOffset(uni->type, offset) || offset > uni->count)
        break;

      return MakeLocation(i, offset, shader->flags & SHADER_FLAG_GEOMETRY);
    }
  }

  return -1;
}

#define SetInt GLASS_uniforms_setInt
static void GLASS_uniforms_setInt(const GLint location, const GLint *values,
                                  const size_t numOfComponents,
                                  const size_t numOfElements) {}

#define SetFloat GLASS_uniforms_setFloat
static void GLASS_uniforms_setFloat(const GLint location, const GLfloat *values,
                                    const size_t numOfComponents,
                                    const size_t numOfElements) {}

// Uniforms

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
                        GLsizei *length, GLint *size, GLenum *type,
                        GLchar *name) {
  if (bufSize)
    Assert(name, "Name was nullptr!");

  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  if (bufSize < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  ProgramInfo *prog = (ProgramInfo *)program;

  // Get shader.
  ShaderInfo *shad = (ShaderInfo *)prog->linkedVertex;
  if (!shad)
    return;

  if (index > shad->numOfActiveUniforms) {
    index -= shad->numOfActiveUniforms;
    shad = (ShaderInfo *)prog->linkedGeometry;
  }

  if (!shad)
    return;

  if (index > shad->numOfActiveUniforms) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Get uniform data.
  const UniformInfo *uni = &shad->activeUniforms[index];
  size_t symLength = Min(bufSize, strlen(uni->symbol));

  if (length)
    *length = symLength;

  *size = uni->count;
  strncpy(name, uni->symbol, symLength);

  switch (uni->type) {
  case GLASS_UNI_BOOL:
    *type = GL_BOOL;
    break;
  case GLASS_UNI_INT:
    *type = GL_INT_VEC4;
    break;
  case GLASS_UNI_FLOAT:
    *type = GL_FLOAT_VEC4;
    break;
  default:
    Unreachable("Invalid uniform type!");
  }
}

void glGetUniformfv(GLuint program, GLint location, GLfloat *params); // TODO
void glGetUniformiv(GLuint program, GLint location, GLint *params);   // TODO

GLint glGetUniformLocation(GLuint program, const GLchar *name) {
  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return -1;
  }

  ProgramInfo *prog = (ProgramInfo *)program;
  if (prog->flags & PROGRAM_FLAG_LINK_FAILED) {
    SetError(GL_INVALID_OPERATION);
    return -1;
  }

  // Get offset.
  const size_t offset = ExtractOffset(name);
  if (offset != -1) {
    // Lookup uniform.
    if (ObjectIsShader(prog->linkedVertex)) {
      ShaderInfo *vshad = (ShaderInfo *)prog->linkedVertex;
      ShaderInfo *gshad = (ShaderInfo *)prog->linkedGeometry;
      GLint loc = LookupUniform(vshad, name, offset);

      if (loc == -1 && gshad)
        loc = LookupUniform(gshad, name, offset);

      return loc;
    }
  }

  return -1;
}

void glUniform1f(GLint location, GLfloat v0) {
  GLfloat values[] = {v0};
  glUniform1fv(location, 1, values);
}

void glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
  GLfloat values[] = {v0, v1};
  glUniform2fv(location, 1, values);
}

void glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
  GLfloat values[] = {v0, v1, v2};
  glUniform3fv(location, 1, values);
}

void glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                 GLfloat v3) {
  GLfloat values[] = {v0, v1, v2, v3};
  glUniform4fv(location, 1, values);
}

void glUniform1i(GLint location, GLint v0) {
  GLint values[] = {v0};
  glUniform1iv(location, 1, values);
}

void glUniform2i(GLint location, GLint v0, GLint v1) {
  GLint values[] = {v0, v1};
  glUniform2iv(location, 1, values);
}

void glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
  GLint values[] = {v0, v1, v2};
  glUniform3iv(location, 1, values);
}

void glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
  GLint values[] = {v0, v1, v2, v3};
  glUniform4iv(location, 1, values);
}

void glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat(location, value, 1, count);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat(location, value, 2, count);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat(location, value, 3, count);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat(location, value, 4, count);
}

void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
  SetInt(location, value, 1, count);
}

void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
  SetInt(location, value, 2, count);
}

void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
  SetInt(location, value, 3, count);
}

void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
  SetInt(location, value, 4, count);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform2fv(location, 2, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform3fv(location, 3, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform4fv(location, 4, value);
}