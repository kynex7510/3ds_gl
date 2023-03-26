#include "context.h"
#include "gpu.h"
#include "utility.h"

#define RemoveBits(mask)                                                       \
  (((((mask) & ~GL_COLOR_BUFFER_BIT) & ~GL_DEPTH_BUFFER_BIT) &                 \
    ~GL_STENCIL_BUFFER_BIT) &                                                  \
   ~GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define HasColor(mask) ((mask)&GL_COLOR_BUFFER_BIT)
#define HasDepth(mask) ((mask)&GL_DEPTH_BUFFER_BIT)
#define HasStencil(mask) ((mask)&GL_STENCIL_BUFFER_BIT)
#define HasEarlyDepth(mask) ((mask)&GL_EARLY_DEPTH_BUFFER_BIT_PICA)

#define IsDrawMode(mode)                                                       \
  (((mode) == GL_TRIANGLES) || ((mode) == GL_TRIANGLE_STRIP) ||                \
   ((mode) == GL_TRIANGLE_FAN) || ((mode) == GL_GEOMETRY_PRIMITIVE_PICA))

#define IsElementsType(type)                                                   \
  (((type) == GL_UNSIGNED_BYTE) || ((type) == GL_UNSIGNED_SHORT))

// Helpers

#define CheckFB GLASS_rendering_checkFB
static bool GLASS_rendering_checkFB(void) {
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    SetError(GL_INVALID_FRAMEBUFFER_OPERATION);
    return false;
  }

  return true;
}

// Rendering

void glClear(GLbitfield mask) {
  // Check parameters.
  if (RemoveBits(mask) || (!HasDepth(mask) && HasStencil(mask))) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (!CheckFB())
    return;

  CtxImpl *ctx = GetContext();

  // Clear early depth buffer.
  if (HasEarlyDepth(mask))
    ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH_CLEAR;

  if (HasColor(mask) || HasDepth(mask)) {
    // Early depth clear is a GPU command, and its execution does not need
    // to be enforced before a clear call is issued. This doesn't apply to
    // other renderbuffers which rely on a GX call.
    ctx = UpdateContext();
    GPUFlushCommands(ctx);

    // Clear framebuffer.
    FramebufferInfo *fb = (FramebufferInfo *)ctx->framebuffer;
    RenderbufferInfo *colorBuffer = HasColor(mask) ? fb->colorBuffer : NULL;
    u32 clearColor =
        colorBuffer ? ConvertRGBA8(colorBuffer->format, ctx->clearColor) : 0;
    RenderbufferInfo *depthBuffer = HasDepth(mask) ? fb->depthBuffer : NULL;
    u32 clearDepth = depthBuffer
                         ? GetClearDepth(depthBuffer->format, ctx->clearDepth,
                                         ctx->clearStencil)
                         : 0;
    ClearBuffers(colorBuffer, clearColor, depthBuffer, clearDepth);
  }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  CtxImpl *ctx = GetContext();
  ctx->clearColor = (uint32_t)(0xFF * GLClampFloat(red)) << 24;
  ctx->clearColor |= (uint32_t)(0xFF * GLClampFloat(green)) << 16;
  ctx->clearColor |= (uint32_t)(0xFF * GLClampFloat(blue)) << 8;
  ctx->clearColor |= (uint32_t)(0xFF * GLClampFloat(alpha));
}

void glClearDepthf(GLclampf depth) {
  CtxImpl *ctx = GetContext();
  ctx->clearDepth = GLClampFloat(depth);
}

void glClearStencil(GLint s) {
  CtxImpl *ctx = GetContext();
  ctx->clearStencil = (uint8_t)s;
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
  if (!IsDrawMode(mode)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (count < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (!CheckFB())
    return;

  // Apply prior commands.
  CtxImpl *ctx = UpdateContext();

  // Add draw command.
  GPUEnableRegs(ctx);
  DrawArrays(mode, first, count);
  GPUDisableRegs(ctx);
  ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type,
                    const GLvoid *indices) {
  if (!IsDrawMode(mode)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (!IsElementsType(type)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (count < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  if (!CheckFB())
    return;

  // Apply prior commands.
  CtxImpl *ctx = UpdateContext();

  // Add draw command.
  GPUEnableRegs(ctx);
  DrawElements(mode, count, type, indices);
  GPUDisableRegs(ctx);
  ctx->flags |= CONTEXT_FLAG_DRAW;
}

void glFinish(void) { GPUFlushAndRunCommands(UpdateContext()); }
void glFlush(void) { GPUFlushCommands(UpdateContext()); }