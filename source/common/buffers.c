#include "context.h"
#include "utility.h"

// Helpers

#define GetBoundBufferInfo GLASS_buffers_getBoundBufferInfo
static BufferInfo *GLASS_buffers_getBoundBufferInfo(const GLenum target) {
  GLuint buffer = GLASS_INVALID_OBJECT;
  CtxImpl *ctx = GetContext();

  switch (target) {
  case GL_ARRAY_BUFFER:
    buffer = ctx->arrayBuffer;
    break;
  case GL_ELEMENT_ARRAY_BUFFER:
    buffer = ctx->elementArrayBuffer;
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return NULL;
  }

  if (buffer != GLASS_INVALID_OBJECT)
    return (BufferInfo *)buffer;

  SetError(GL_INVALID_OPERATION);
  return NULL;
}

// Buffers

void glBindBuffer(GLenum target, GLuint buffer) {
  if (!ObjectIsBuffer(buffer) && buffer != GLASS_INVALID_OBJECT) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  BufferInfo *info = (BufferInfo *)buffer;
  CtxImpl *ctx = GetContext();

  // Bind buffer to context.
  switch (target) {
  case GL_ARRAY_BUFFER:
    ctx->arrayBuffer = buffer;
    break;
  case GL_ELEMENT_ARRAY_BUFFER:
    ctx->elementArrayBuffer = buffer;
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (info)
    info->flags |= BUFFER_FLAG_BOUND;
}

void glBufferData(GLenum target, GLsizeiptr size, const void *data,
                  GLenum usage) {
  if (usage != GL_STREAM_DRAW && usage != GL_STATIC_DRAW &&
      usage != GL_DYNAMIC_DRAW) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (size < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Get buffer.
  BufferInfo *info = GetBoundBufferInfo(target);
  if (!info)
    return;

  // Allocate buffer.
  if (info->address)
    linearFree(info->address);

  info->address = linearAlloc(size);

  if (!info->address) {
    SetError(GL_OUT_OF_MEMORY);
    return;
  }

  info->usage = usage;

  if (data)
    CopyMem(data, info->address, size);
}

void glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size,
                     const void *data) {
  Assert(data, "Data was nullptr!");

  if (offset < 0 || size < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Get buffer.
  BufferInfo *info = GetBoundBufferInfo(target);
  if (!info)
    return;

  // Get buffer size.
  GLsizeiptr bufSize = (GLsizeiptr)linearGetSize(info->address);

  if (size > (bufSize - offset)) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Copy data.
  CopyMem(data, info->address + offset, size);
}

void glDeleteBuffers(GLsizei n, const GLuint *buffers) {
  Assert(buffers, "Buffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = buffers[i];

    // Validate name.
    if (!ObjectIsBuffer(name))
      continue;

    BufferInfo *info = (BufferInfo *)name;

    // Unbind if bound.
    if (ctx->arrayBuffer == name)
      ctx->arrayBuffer = GLASS_INVALID_OBJECT;
    else if (ctx->elementArrayBuffer == name)
      ctx->elementArrayBuffer = GLASS_INVALID_OBJECT;

    // Delete buffer.
    if (info->address)
      linearFree(info->address);

    FreeMem(info);
  }
}

void glGenBuffers(GLsizei n, GLuint *buffers) {
  Assert(buffers, "Buffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = CreateObject(GLASS_BUFFER_TYPE);

    if (name == GLASS_INVALID_OBJECT) {
      SetError(GL_OUT_OF_MEMORY);
      return;
    }

    BufferInfo *info = (BufferInfo *)name;
    info->address = NULL;
    info->usage = GL_STATIC_DRAW;
    info->flags = 0;
    buffers[i] = name;
  }
}

void glGetBufferParameteriv(GLenum target, GLenum pname, GLint *data) {
  Assert(data, "Data was nullptr!");

  BufferInfo *info = GetBoundBufferInfo(target);
  if (!info)
    return;

  // Get parameter.
  switch (pname) {
  case GL_BUFFER_SIZE:
    *data = linearGetSize(info->address);
    break;
  case GL_BUFFER_USAGE:
    *data = info->usage;
    break;
  default:
    SetError(GL_INVALID_ENUM);
  }
}

GLboolean glIsBuffer(GLuint buffer) {
  if (ObjectIsBuffer(buffer)) {
    BufferInfo *info = (BufferInfo *)buffer;
    if (info->flags & BUFFER_FLAG_BOUND)
      return GL_TRUE;
  }

  return GL_FALSE;
}