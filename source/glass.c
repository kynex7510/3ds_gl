#include "context.h"
#include "gpu.h"
#include "utility.h"

// Helpers

#define GetDisplayBuffer GLASS_internal_getDisplayBuffer
static void GLASS_internal_getDisplayBuffer(CtxImpl *ctx,
                                            RenderbufferInfo *displayBuffer) {
  u16 width = 0, height = 0;
  displayBuffer->address =
      gfxGetFramebuffer(ctx->targetScreen, ctx->targetSide, &height, &width);
  displayBuffer->format =
      GSPToGLFBFormat(gfxGetScreenFormat(ctx->targetScreen));
  displayBuffer->width = width;
  displayBuffer->height = height;
}

#define SwapBuffers GLASS_internal_swapBuffers
static void GLASS_internal_swapBuffers(gxCmdQueue_s *queue) {
  CtxImpl *ctx = (CtxImpl *)queue->user;
  gfxScreenSwapBuffers(ctx->targetScreen, true /* TODO */);
  gxCmdQueueSetCallback(queue, NULL, NULL);
}

// GLASS

void glassSetAllocator(const glassAllocator allocator,
                       const glassDeallocator deallocator) {
  SetMemAlloc(allocator, deallocator);
}

glassCtx *glassCreateContext(void) {
  CtxImpl *ctx = (CtxImpl *)AllocMem(sizeof(CtxImpl));

  if (ctx)
    InitContext(ctx);

  return (glassCtx *)ctx;
}

void glassDestroyContext(glassCtx *ctx) {
  Assert(ctx, "Context was nullptr!");
  FiniContext((CtxImpl *)ctx);
  ZeroVar(*(CtxImpl *)ctx);
  FreeMem(ctx);
}

void glassBindContext(glassCtx *ctx) { BindContext((CtxImpl *)ctx); }

void glassSwapBuffers(void) {
  // Execute GPU commands.
  CtxImpl *ctx = UpdateContext();
  GPUFlushAndRunCommands(ctx);

  // Framebuffer might be not set.
  if (ObjectIsFramebuffer(ctx->framebuffer)) {
    // Get color buffer.
    FramebufferInfo *fb = (FramebufferInfo *)ctx->framebuffer;
    RenderbufferInfo *colorBuffer = fb->colorBuffer;

    if (colorBuffer) {
      // Get display buffer.
      RenderbufferInfo displayBuffer;
      ZeroVar(displayBuffer);
      GetDisplayBuffer(ctx, &displayBuffer);
      Assert(displayBuffer.address, "Display buffer was nullptr!");

      // Get transfer flags.
      const u32 transferFlags = BuildTransferFlags(
          false, false, false, GLToGXFBFormat(fb->colorBuffer->format),
          GLToGXFBFormat(displayBuffer.format), ctx->transferScale);

      // Transfer buffer.
      gxCmdQueueWait(&ctx->gxQueue, -1);
      gxCmdQueueSetCallback(&ctx->gxQueue, SwapBuffers, (void *)ctx);
      TransferBuffer(fb->colorBuffer, &displayBuffer, transferFlags);
    }
  }
}