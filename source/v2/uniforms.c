/**
 *
 */
#include "context.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

#define MAKE_LOC(id, offset) (((id) << 8) | (offset))
#define UNWRAP_LOC(loc) (((loc) >> 8) + ((loc)&0xFF))

// Helpers

#define ExtractOffset GLASS_uniforms_extractOffset
static size_t GLASS_uniforms_extractOffset(const char *name) {
  size_t offset = 0;

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
  for (size_t i = 0; i < vshad->numOfUniforms; i++) {
    const UniformInfo *info = &vshad->uniformInfo[i];
    if (!uni->symbol)
      continue;

    if (strstr(uni->symbol, name) == uni->symbol) {
      if (((uni->type == GLASS_UNI_BOOL) && (offset > 15)) ||
          ((uni->type == GLASS_UNI_INT) && (offset > 3)) ||
          ((uni->type == GLASS_UNI_FLOAT) && (offset > 95)))
        break;

      return MAKE_LOC(i, offset);
    }
  }

  return -1;
}

static void SetFloat(UniformInfo *info) {}

// Uniforms

void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize,
                        GLsizei *length, GLint *size, GLenum *type,
                        GLchar *name); // TODO

void glGetUniformfv(GLuint program, GLint location, GLfloat *params); // TODO
void glGetUniformiv(GLuint program, GLint location, GLint *params);   // TODO

GLint glGetUniformLocation(GLuint program, const GLchar *name) {
  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *prog = (ProgramInfo *)program;
  if (prog->flags & PROGRAM_FLAG_LINK_FAILED) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  // Lookup uniform.
  Assert(ObjectIsShader(info->attachedVertex), "Invalid vertex shader!");
  ShaderInfo *vshad = (ShaderInfo *)prog->linkedVertex;
  ShaderInfo *gshad = (ShaderInfo *)prog->linkedGeometry;
  GLint loc = LookupUniform(vshad, name);

  if (loc == -1 && gshad)
    loc = LookupUniform(gshad, name);

  return loc;
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
  SetFloat((UniformInfo *)location, value, 1, count);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat((UniformInfo *)location, value, 2, count);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat((UniformInfo *)location, value, 3, count);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
  SetFloat((UniformInfo *)location, value, 4, count);
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
                        const GLfloat *value); // TODO
void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value); // TODO
void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value); // TODO