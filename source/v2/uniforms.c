#include "context.h"
#include "utility.h"

#include <stdlib.h>
#include <string.h>

#define CheckOffset(type, offset)                                              \
  ((((type) == GLASS_UNI_BOOL) && ((offset) < GLASS_NUM_BOOL_UNIFORMS)) ||     \
   (((type) == GLASS_UNI_INT) && ((offset) < GLASS_NUM_INT_UNIFORMS)) ||       \
   (((type) == GLASS_UNI_FLOAT) && ((offset) < GLASS_NUM_FLOAT_UNIFORMS)))

// Helpers

#define MakeLocation GLASS_uniforms_makeLocation
static GLint GLASS_uniforms_makeLocation(const size_t index,
                                         const size_t offset,
                                         const bool isGeometry) {
  return (GLint)((isGeometry << 16) | (index << 8) | (offset & 0xFF));
}

#define GetLocInfo GLASS_uniforms_getLocInfo
static bool GLASS_uniforms_getLocInfo(const GLint loc, size_t *index,
                                      size_t *offset, bool *isGeometry) {
  Assert(index, "Index was nullptr!");
  Assert(offset, "Offset was nullptr!");
  Assert(isGeometry, "IsGeometry was nullptr!");

  if (loc != -1) {
    *index = (loc >> 8) & 0xFF;
    *offset = loc & 0xFF;
    *isGeometry = (loc >> 16) & 1;
    return true;
  }

  return false;
}

#define ExtractOffset GLASS_uniforms_extractOffset
static size_t GLASS_uniforms_extractOffset(const char *name) {
  Assert(name, "Name was nullptr!");

  if (strstr(name, ".") || (strstr(name, "gl_") == name))
    return -1;

  const char *beg = strstr(name, "[");
  if (!beg)
    return 0;

  const char *end = strstr(beg, "]");
  if (!end || end[1] != '\0' || !(end - beg))
    return -1;

  return atoi(&beg[1]);
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

#define GetShaderUniform GLASS_uniforms_getShaderUniform
static UniformInfo *GLASS_uniforms_getShaderUniform(const ProgramInfo *program,
                                                    const size_t index,
                                                    const bool isGeometry) {
  Assert(program, "Program was nullptr!");

  ShaderInfo *shader = NULL;
  if (isGeometry) {
    if (!ObjectIsShader(program->linkedGeometry))
      return NULL;

    shader = (ShaderInfo *)program->linkedGeometry;
  } else {
    if (!ObjectIsShader(program->linkedVertex))
      return NULL;

    shader = (ShaderInfo *)program->linkedVertex;
  }

  if (index > shader->numOfActiveUniforms)
    return NULL;

  return &shader->activeUniforms[index];
}

#define SetValues GLASS_uniforms_setValues
static void GLASS_uniforms_setValues(const GLint location,
                                     const GLint *intValues,
                                     const GLfloat *floatValues,
                                     const size_t numOfComponents,
                                     const GLsizei numOfElements) {
  Assert(numOfComponents <= 4, "Invalid num of components!");

  if (numOfElements < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Parse location.
  size_t locIndex = 0;
  size_t locOffset = 0;
  bool locIsGeometry = 0;
  if (!GetLocInfo(location, &locIndex, &locOffset, &locIsGeometry))
    return; // A value of -1 is silently ignored.

  // Get program.
  CtxImpl *ctx = GetContext();
  if (!ObjectIsProgram(ctx->currentProgram)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *prog = (ProgramInfo *)ctx->currentProgram;

  // Get uniform.
  UniformInfo *uni = GetShaderUniform(prog, locIndex, locIsGeometry);
  if (!uni || locOffset >= uni->count ||
      (uni->count == 1 && numOfElements != 1)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  // Handle bool.
  if (uni->type == GLASS_UNI_BOOL) {
    if (numOfComponents != 1) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    for (size_t i = locOffset; i < Min(uni->count, locOffset + numOfElements);
         i++) {
      if (intValues) {
        SetBoolUniform(uni, i, intValues[i] != 0);
      } else if (floatValues) {
        SetBoolUniform(uni, i, floatValues[i] != 0.0f);
      } else {
        Unreachable("Value buffer was nullptr!");
      }
    }

    return;
  }

  // Handle int.
  if (uni->type == GLASS_UNI_INT) {
    if (!intValues) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    for (size_t i = locOffset; i < Min(uni->count, locOffset + numOfElements);
         i++) {
      u32 components[4] = {};
      u32 packed = 0;

      GetIntUniform(uni, i, &packed);
      UnpackIntVector(packed, components);

      for (size_t j = 0; j < numOfComponents; j++)
        components[j] = intValues[(numOfComponents * (i - locOffset)) + j];

      PackIntVector(components, &packed);
      SetIntUniform(uni, i, packed);
    }

    return;
  }

  // Handle float.
  if (uni->type == GLASS_UNI_FLOAT) {
    if (!floatValues) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    for (size_t i = locOffset; i < Min(uni->count, locOffset + numOfElements);
         i++) {
      float components[4] = {};
      u32 packed[3] = {};

      GetFloatUniform(uni, i, packed);
      UnpackFloatVector(packed, components);

      for (size_t j = 0; j < numOfComponents; j++)
        components[j] = floatValues[(numOfComponents * (i - locOffset)) + j];

      PackFloatVector(components, packed);
      SetFloatUniform(uni, i, packed);
    }

    return;
  }

  Unreachable("Invalid uniform type!");
}

#define GetValues GLASS_uniforms_getValues
void GLASS_uniforms_getValues(GLuint program, GLint location, GLint *intParams,
                              GLfloat *floatParams) {
  Assert(intParams || floatParams, "Params buffer was nullptr!");

  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *prog = (ProgramInfo *)program;

  // Parse location.
  size_t locIndex = 0;
  size_t locOffset = 0;
  bool locIsGeometry = 0;
  if (!GetLocInfo(location, &locIndex, &locOffset, &locIsGeometry)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  // Get uniform.
  UniformInfo *uni = GetShaderUniform(prog, locIndex, locIsGeometry);
  if (!uni) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  // Handle bool.
  if (uni->type == GLASS_UNI_BOOL) {
    if (intParams)
      intParams[0] = GetBoolUniform(uni, locOffset) ? 1 : 0;
    else
      floatParams[0] = GetBoolUniform(uni, locOffset) ? 1.0f : 0.0f;

    return;
  }

  // Handle int.
  if (uni->type == GLASS_UNI_INT) {
    u32 packed = 0;
    u32 components[4] = {};
    GetIntUniform(uni, locOffset, &packed);
    UnpackIntVector(packed, components);

    if (intParams) {
      for (size_t i = 0; i < 4; i++)
        intParams[i] = (GLint)components[i];
    } else {
      for (size_t i = 0; i < 4; i++)
        floatParams[i] = (GLfloat)components[i];
    }

    return;
  }

  // Handle float.
  if (uni->type == GLASS_UNI_FLOAT) {
    u32 packed[3] = {};
    GetFloatUniform(uni, locOffset, packed);

    if (floatParams) {
      UnpackFloatVector(packed, floatParams);
    } else {
      float components[4] = {};
      UnpackFloatVector(packed, components);
      for (size_t i = 0; i < 4; i++)
        intParams[i] = (GLint)components[i];
    }

    return;
  }

  Unreachable("Invalid uniform type!");
}

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

void glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
  GetValues(program, location, NULL, params);
}

void glGetUniformiv(GLuint program, GLint location, GLint *params) {
  GetValues(program, location, params, NULL);
}

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
  SetValues(location, NULL, value, 1, count);
}

void glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
  SetValues(location, NULL, value, 2, count);
}

void glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
  SetValues(location, NULL, value, 3, count);
}

void glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
  SetValues(location, NULL, value, 4, count);
}

void glUniform1iv(GLint location, GLsizei count, const GLint *value) {
  SetValues(location, value, NULL, 1, count);
}

void glUniform2iv(GLint location, GLsizei count, const GLint *value) {
  SetValues(location, value, NULL, 2, count);
}

void glUniform3iv(GLint location, GLsizei count, const GLint *value) {
  SetValues(location, value, NULL, 3, count);
}

void glUniform4iv(GLint location, GLsizei count, const GLint *value) {
  SetValues(location, value, NULL, 4, count);
}

void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform2fv(location, 2 * count, value);
}

void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform3fv(location, 3 * count, value);
}

void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose,
                        const GLfloat *value) {
  if (transpose != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  glUniform4fv(location, 4 * count, value);
}