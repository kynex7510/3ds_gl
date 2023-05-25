#include "context.h"
#include "gpu.h"
#include "utility.h"

// Helpers

#define GetDisplayBuffer GLASS_internal_getDisplayBuffer
static void GLASS_internal_getDisplayBuffer(CtxImpl *ctx,
                                            RenderbufferInfo *displayBuffer) {
  u16 width = 0, height = 0;
  displayBuffer->address = gfxGetFramebuffer(
      ctx->exposed.targetScreen, ctx->exposed.targetSide, &height, &width);
  displayBuffer->format =
      WrapFBFormat(gfxGetScreenFormat(ctx->exposed.targetScreen));
  displayBuffer->width = width;
  displayBuffer->height = height;
}

#define SwapBuffers GLASS_internal_swapBuffers
static void GLASS_internal_swapBuffers(gxCmdQueue_s *queue) {
  CtxImpl *ctx = (CtxImpl *)queue->user;
  gfxScreenSwapBuffers(ctx->exposed.targetScreen,
                       ctx->exposed.targetScreen == GFX_TOP &&
                           ctx->exposed.targetSide == GFX_RIGHT);
  gxCmdQueueSetCallback(queue, NULL, NULL);
}

// GLASS

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
  RenderbufferInfo displayBuffer;
  ZeroVar(displayBuffer);

  // Execute GPU commands.
  CtxImpl *ctx = UpdateContext();
  GPUFlushAndRunCommands(ctx);

  // Framebuffer might not be set.
  if (!ObjectIsFramebuffer(ctx->framebuffer))
    return;

  // Get color buffer.
  FramebufferInfo *fb = (FramebufferInfo *)ctx->framebuffer;
  RenderbufferInfo *colorBuffer = fb->colorBuffer;
  if (!colorBuffer)
    return;

  // Get display buffer.
  GetDisplayBuffer(ctx, &displayBuffer);
  Assert(displayBuffer.address, "Display buffer was nullptr!");

  // Get transfer flags.
  const u32 transferFlags = MakeTransferFlags(
      false, false, false, GetTransferFormatForFB(fb->colorBuffer->format),
      GetTransferFormatForFB(displayBuffer.format), ctx->exposed.transferScale);

  // Transfer buffer.
  GPUFlushQueue(ctx, false);
  gxCmdQueueSetCallback(&ctx->gxQueue, SwapBuffers, (void *)ctx);
  GPUTransferBuffer(fb->colorBuffer, &displayBuffer, transferFlags);
  GPURunQueue(ctx, false);
}