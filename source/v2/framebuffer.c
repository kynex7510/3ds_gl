#include "context.h"
#include "utility.h"

#define IsColorFormat(format)                                                  \
  (((format) == GL_RGBA8_OES) || ((format) == GL_RGB5_A1) ||                   \
   ((format) == GL_RGB565) || ((format) == GL_RGBA4))

#define IsDepthFormat(format)                                                  \
  (((format) == GL_DEPTH_COMPONENT16) ||                                       \
   ((format) == GL_DEPTH_COMPONENT24_OES) ||                                   \
   ((format) == GL_DEPTH24_STENCIL8_EXT))

// Helpers

#define GetColorSize GLASS_framebuffer_getColorSize
static GLint GLASS_framebuffer_getColorSize(const GLenum format,
                                            const GLenum color) {
  switch (format) {
  case GL_RGBA8_OES:
    return 8;
  case GL_RGB5_A1:
    return color == GL_RENDERBUFFER_ALPHA_SIZE ? 1 : 5;
  case GL_RGB565:
    if (color == GL_RENDERBUFFER_GREEN_SIZE)
      return 6;
    else if (color == GL_RENDERBUFFER_ALPHA_SIZE)
      return 0;
    else
      return 5;
  case GL_RGBA4:
    return 4;
  }

  Unreachable("Invalid color format!");
}

#define GetDepthSize GLASS_framebuffer_getDepthSize
static GLint GLASS_framebuffer_getDepthSize(const GLenum format) {
  switch (format) {
  case GL_DEPTH_COMPONENT16:
    return 16;
  case GL_DEPTH_COMPONENT24_OES:
  case GL_DEPTH24_STENCIL8_EXT:
    return 24;
  }

  Unreachable("Invalid depth format!");
}

// Framebuffer

void glBindFramebuffer(GLenum target, GLuint framebuffer) {
  if (target != GL_FRAMEBUFFER) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (!ObjectIsFramebuffer(framebuffer) &&
      framebuffer != GLASS_INVALID_OBJECT) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  FramebufferInfo *info = (FramebufferInfo *)framebuffer;
  CtxImpl *ctx = GetContext();

  // Bind framebuffer to context.
  if (ctx->framebuffer != framebuffer) {
    ctx->framebuffer = framebuffer;

    if (info)
      info->flags |= FRAMEBUFFER_FLAG_BOUND;

    ctx->flags |= CONTEXT_FLAG_FRAMEBUFFER;
  }
}

void glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
  if (target != GL_RENDERBUFFER) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (!ObjectIsRenderbuffer(renderbuffer) &&
      renderbuffer != GLASS_INVALID_OBJECT) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  RenderbufferInfo *info = (RenderbufferInfo *)renderbuffer;
  CtxImpl *ctx = GetContext();

  // Bind renderbuffer to context.
  if (ctx->renderbuffer != renderbuffer) {
    ctx->renderbuffer = renderbuffer;

    if (info)
      info->flags |= RENDERBUFFER_FLAG_BOUND;
  }
}

GLenum glCheckFramebufferStatus(GLenum target) {
  if (target != GL_FRAMEBUFFER) {
    SetError(GL_INVALID_ENUM);
    return 0;
  }

  CtxImpl *ctx = GetContext();

  // Make sure we have a framebuffer.
  if (!ObjectIsFramebuffer(ctx->framebuffer))
    return GL_FRAMEBUFFER_UNSUPPORTED;

  FramebufferInfo *info = (FramebufferInfo *)ctx->framebuffer;

  // Check that we have at least one attachment.
  if (!info->colorBuffer && !info->depthBuffer)
    return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;

  // Check buffers.
  if (info->colorBuffer) {
    if (!info->colorBuffer->address)
      return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
  }

  if (info->depthBuffer) {
    if (!info->depthBuffer->address)
      return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
  }

  if (info->colorBuffer && info->depthBuffer) {
    if ((info->colorBuffer->width != info->depthBuffer->width) ||
        (info->colorBuffer->height != info->depthBuffer->height))
      return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
  }

  return GL_FRAMEBUFFER_COMPLETE;
}

void glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
  Assert(framebuffers, "Framebuffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = framebuffers[i];

    // Validate name.
    if (!ObjectIsFramebuffer(name))
      continue;

    // Unbind if bound.
    if (ctx->framebuffer == name)
      ctx->framebuffer = GLASS_INVALID_OBJECT;

    // Delete framebuffer.
    FreeMem((FramebufferInfo *)name);
  }
}

void glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
  Assert(renderbuffers, "Renderbuffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  // Get framebuffer.
  FramebufferInfo *fbinfo = NULL;
  if (ObjectIsFramebuffer(ctx->framebuffer))
    fbinfo = (FramebufferInfo *)ctx->framebuffer;

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = renderbuffers[i];

    // Validate name.
    if (!ObjectIsRenderbuffer(name))
      continue;

    RenderbufferInfo *info = (RenderbufferInfo *)name;

    // Unbind if bound.
    if (fbinfo) {
      if (fbinfo->colorBuffer == info)
        fbinfo->colorBuffer = NULL;
      else if (fbinfo->depthBuffer == info)
        fbinfo->depthBuffer = NULL;
    }

    // Delete renderbuffer.
    if (info->address)
      FreeVRAM(info->address);

    FreeMem(info);
  }
}

void glFramebufferRenderbuffer(GLenum target, GLenum attachment,
                               GLenum renderbuffertarget, GLuint renderbuffer) {
  if (target != GL_FRAMEBUFFER || renderbuffertarget != GL_RENDERBUFFER) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (!ObjectIsRenderbuffer(renderbuffer) &&
      renderbuffer != GLASS_INVALID_OBJECT) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  RenderbufferInfo *rbinfo = (RenderbufferInfo *)renderbuffer;
  CtxImpl *ctx = GetContext();

  // Get framebuffer.
  if (!ObjectIsFramebuffer(ctx->framebuffer)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  FramebufferInfo *fbinfo = (FramebufferInfo *)ctx->framebuffer;

  // Set right buffer.
  switch (attachment) {
  case GL_COLOR_ATTACHMENT0:
    fbinfo->colorBuffer = rbinfo;
    break;
  case GL_DEPTH_ATTACHMENT:
  case GL_STENCIL_ATTACHMENT:
    fbinfo->depthBuffer = rbinfo;
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return;
  }

  ctx->flags |= CONTEXT_FLAG_FRAMEBUFFER;
}

void glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
  Assert(framebuffers, "Framebuffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = CreateObject(GLASS_FRAMEBUFFER_TYPE);
    if (!ObjectIsFramebuffer(name)) {
      SetError(GL_OUT_OF_MEMORY);
      return;
    }

    framebuffers[i] = name;
  }
}

void glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
  Assert(renderbuffers, "Renderbuffers was nullptr!");

  if (n < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  for (GLsizei i = 0; i < n; i++) {
    GLuint name = CreateObject(GLASS_RENDERBUFFER_TYPE);
    if (!ObjectIsRenderbuffer(name)) {
      SetError(GL_OUT_OF_MEMORY);
      return;
    }

    RenderbufferInfo *info = (RenderbufferInfo *)name;
    info->format = GL_RGBA4;
    renderbuffers[i] = name;
  }
}

void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
  Assert(params, "Params was nullptr!");

  if (target != GL_RENDERBUFFER) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();

  // Get renderbuffer.
  if (!ObjectIsRenderbuffer(ctx->renderbuffer)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  RenderbufferInfo *info = (RenderbufferInfo *)ctx->renderbuffer;

  switch (pname) {
  case GL_RENDERBUFFER_WIDTH:
    *params = info->width;
    break;
  case GL_RENDERBUFFER_HEIGHT:
    *params = info->height;
    break;
  case GL_RENDERBUFFER_INTERNAL_FORMAT:
    *params = info->format;
    break;
  case GL_RENDERBUFFER_RED_SIZE:
  case GL_RENDERBUFFER_GREEN_SIZE:
  case GL_RENDERBUFFER_BLUE_SIZE:
  case GL_RENDERBUFFER_ALPHA_SIZE:
    *params = GetColorSize(info->format, pname);
    break;
  case GL_RENDERBUFFER_DEPTH_SIZE:
    *params = GetDepthSize(info->format);
    break;
  case GL_RENDERBUFFER_STENCIL_SIZE:
    *params = info->format == GL_DEPTH24_STENCIL8_EXT ? 8 : 0;
    break;
  default:
    SetError(GL_INVALID_ENUM);
  }
}

GLboolean glIsFramebuffer(GLuint framebuffer) {
  if (ObjectIsFramebuffer(framebuffer)) {
    FramebufferInfo *info = (FramebufferInfo *)framebuffer;
    if (info->flags & FRAMEBUFFER_FLAG_BOUND)
      return GL_TRUE;
  }

  return GL_FALSE;
}

GLboolean glIsRenderbuffer(GLuint renderbuffer) {
  if (ObjectIsRenderbuffer(renderbuffer)) {
    RenderbufferInfo *info = (RenderbufferInfo *)renderbuffer;
    if (info->flags & RENDERBUFFER_FLAG_BOUND)
      return GL_TRUE;
  }

  return GL_FALSE;
}

void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width,
                           GLsizei height) {
  if (target != GL_RENDERBUFFER ||
      (!IsColorFormat(internalformat) && !IsDepthFormat(internalformat))) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (width <= 0 || height <= 0 || width > 1024 || height > 1024) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();

  // Get renderbuffer.
  if (!ObjectIsRenderbuffer(ctx->renderbuffer)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  RenderbufferInfo *info = (RenderbufferInfo *)ctx->renderbuffer;

  // Allocate buffer.
  const size_t bufferSize = width * height * GetBytesPerPixel(internalformat);

  if (info->address)
    FreeVRAM(info->address);

  info->address = AllocVRAM(
      bufferSize, IsDepthFormat(internalformat) ? VRAM_ALLOC_B : VRAM_ALLOC_A);

  if (!info->address) {
    info->address =
        AllocVRAM(bufferSize,
                  IsDepthFormat(internalformat) ? VRAM_ALLOC_A : VRAM_ALLOC_B);
  }

  if (!info->address) {
    SetError(GL_OUT_OF_MEMORY);
    return;
  }

  info->width = width;
  info->height = height;
  info->format = internalformat;
}