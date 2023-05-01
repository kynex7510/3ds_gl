#include "context.h"
#include "utility.h"

#define CheckType(type)                                                        \
  (((type) == GL_BYTE) || ((type) == GL_UNSIGNED_BYTE) ||                      \
   ((type) == GL_SHORT) || ((type) == GL_FLOAT))

// Helpers

#define ReadFloats GLASS_attribs_readFloats
static bool GLASS_attribs_readFloats(const size_t index, const GLenum pname,
                                     GLfloat *params) {
  CtxImpl *ctx = GetContext();
  const AttributeInfo *attrib = &ctx->attribs[index];

  if (pname == GL_CURRENT_VERTEX_ATTRIB) {
    for (size_t i = 0; i < 4; i++)
      params[i] = attrib->components[i];
    return true;
  }

  return false;
}

#define ReadInt GLASS_attribs_readInt
static bool GLASS_attribs_readInt(const size_t index, const GLenum pname,
                                  GLint *param) {
  CtxImpl *ctx = GetContext();
  const AttributeInfo *attrib = &ctx->attribs[index];

  switch (pname) {
  case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
    *param = attrib->boundBuffer;
    return true;
  case GL_VERTEX_ATTRIB_ARRAY_SIZE:
    *param = attrib->count;
    return true;
  case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
    *param = attrib->stride;
    return true;
  case GL_VERTEX_ATTRIB_ARRAY_TYPE:
    *param = attrib->type;
    return true;
  case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
    *param = GL_FALSE;
    return true;
  case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
    *param = GL_FALSE;
    for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; i++) {
      if (ctx->attribSlots[i] == index) {
        *param = GL_TRUE;
        break;
      }
    }
    return true;
  }

  return false;
}

#define SetFixedAttrib GLASS_attribs_setFixedAttrib
static void GLASS_attribs_setFixedAttrib(const GLuint reg,
                                         const GLfloat *params) {
  if (reg >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();
  AttributeInfo *attrib = &ctx->attribs[reg];

  // Set fixed attribute.
  attrib->type = GL_FLOAT;
  attrib->count = 4;
  attrib->stride = sizeof(GLfloat) * 4;
  attrib->boundBuffer = 0;
  attrib->physAddr = 0;

  for (size_t i = 0; i < 4; i++)
    attrib->components[i] = params[i];

  ctx->flags |= CONTEXT_FLAG_ATTRIBS;
}

// Attribs

void glBindAttribLocation(GLuint program, GLuint index,
                          const GLchar *name); // TODO

void glDisableVertexAttribArray(GLuint index) {
  if (index >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; i++) {
    if (ctx->attribSlots[i] == index) {
      ctx->attribSlots[i] = GLASS_NUM_ATTRIB_REGS;
      ctx->flags |= CONTEXT_FLAG_ATTRIBS;
      return;
    }
  }
}

void glEnableVertexAttribArray(GLuint index) {
  if (index >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; i++) {
    if (ctx->attribSlots[i] == GLASS_NUM_ATTRIB_REGS) {
      ctx->attribSlots[i] = index;
      ctx->flags |= CONTEXT_FLAG_ATTRIBS;
      return;
    }
  }

  SetError(GL_OUT_OF_MEMORY);
}

void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize,
                       GLsizei *length, GLint *size, GLenum *type,
                       GLchar *name); // TODO

GLint glGetAttribLocation(GLuint program, const GLchar *name); // TODO

void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
  if (index >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (!ReadFloats(index, pname, params)) {
    GLint param;

    if (!ReadInt(index, pname, &param)) {
      SetError(GL_INVALID_ENUM);
      return;
    }

    params[0] = (GLfloat)param;
  }
}

void glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
  if (index >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (!ReadInt(index, pname, params)) {
    GLfloat components[4];

    if (!ReadFloats(index, pname, components)) {
      SetError(GL_INVALID_ENUM);
      return;
    }

    for (size_t i = 0; i < 4; i++)
      params[i] = (GLint)components[i];
  }
}

void glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid **pointer) {
  if (index >= GLASS_NUM_ATTRIB_REGS) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();
  AttributeInfo *attrib = &ctx->attribs[index];

  // Recover virtual address.
  u8 *virtAddr = NULL;
  if (attrib->physAddr) {
    virtAddr = osConvertPhysToVirt(attrib->physAddr);
    Assert(virtAddr, "Invalid virtual address!");

    // Remove buffer base if required.
    if (attrib->boundBuffer) {
      BufferInfo *binfo = (BufferInfo *)attrib->boundBuffer;
      virtAddr -= (u32)binfo->address;
    }
  }

  *pointer = virtAddr;
}

void glVertexAttrib1f(GLuint index, GLfloat v0) {
  GLfloat values[4] = {v0, 0.0f, 0.0f, 1.0f};
  SetFixedAttrib(index, values);
}

void glVertexAttrib2f(GLuint index, GLfloat v0, GLfloat v1) {
  GLfloat values[4] = {v0, v1, 0.0f, 1.0f};
  SetFixedAttrib(index, values);
}

void glVertexAttrib3f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2) {
  GLfloat values[4] = {v0, v1, v2, 1.0f};
  SetFixedAttrib(index, values);
}

void glVertexAttrib4f(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2,
                      GLfloat v3) {
  GLfloat values[4] = {v0, v1, v2, v3};
  SetFixedAttrib(index, values);
}

void glVertexAttrib1fv(GLuint index, const GLfloat *v) {
  Assert(v, "Attribute data was nullptr!");
  glVertexAttrib1f(index, v[0]);
}

void glVertexAttrib2fv(GLuint index, const GLfloat *v) {
  Assert(v, "Attribute data was nullptr!");
  glVertexAttrib2f(index, v[0], v[1]);
}

void glVertexAttrib3fv(GLuint index, const GLfloat *v) {
  Assert(v, "Attribute data was nullptr!");
  glVertexAttrib3f(index, v[0], v[1], v[2]);
}

void glVertexAttrib4fv(GLuint index, const GLfloat *v) {
  Assert(v, "Attribute data was nullptr!");
  glVertexAttrib4f(index, v[0], v[1], v[2], v[3]);
}

void glVertexAttribPointer(GLuint index, GLint size, GLenum type,
                           GLboolean normalized, GLsizei stride,
                           const GLvoid *pointer) {
  if (!CheckType(type)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (index >= GLASS_NUM_ATTRIB_REGS || size < 1 || size > 4 || stride < 0 ||
      normalized != GL_FALSE) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();
  AttributeInfo *attrib = &ctx->attribs[index];

  // Get vertex buffer physical address.
  u32 physAddr = 0;
  if (ctx->arrayBuffer) {
    BufferInfo *binfo = (BufferInfo *)ctx->arrayBuffer;
    physAddr = osConvertVirtToPhys((void *)(binfo->address + (size_t)pointer));
  } else {
    physAddr = osConvertVirtToPhys(pointer);
  }

  Assert(physAddr, "Invalid physical address!");

  // Set attribute values.
  attrib->type = type;
  attrib->count = size;
  attrib->stride = stride;
  attrib->boundBuffer = ctx->arrayBuffer;
  attrib->physAddr = physAddr;
  attrib->components[0] = 0.0f;
  attrib->components[1] = 0.0f;
  attrib->components[2] = 0.0f;
  attrib->components[3] = 1.0f;

  ctx->flags |= CONTEXT_FLAG_ATTRIBS;
}