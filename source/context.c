#include "context.h"
#include "gpu.h"
#include "utility.h"

#define CONTEXT_FLAG_ALL (~(DECL_FLAG(0) - 1))

// Globals

static CtxImpl *g_Context = NULL;
static CtxImpl *g_OldCtx = NULL;

// Context

void GLASS_context_initContext(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  // Platform.
  ctx->flags = CONTEXT_FLAG_ALL;
  ctx->lastError = GL_NO_ERROR;
  ctx->cmdBuffer = NULL;
  ctx->cmdBufferSize = 0;
  ctx->cmdBufferOffset = 0;
  ZeroVar(ctx->gxQueue);
  ctx->exposed.targetScreen = GFX_TOP;
  ctx->exposed.targetSide = GFX_LEFT;
  ctx->exposed.transferScale = GX_TRANSFER_SCALE_NO;
  GPUInit(ctx);

  // Buffers.
  ctx->arrayBuffer = GLASS_INVALID_OBJECT;
  ctx->elementArrayBuffer = GLASS_INVALID_OBJECT;

  // Framebuffer.
  ctx->framebuffer = GLASS_INVALID_OBJECT;
  ctx->renderbuffer = GLASS_INVALID_OBJECT;
  ctx->clearColor = 0;
  ctx->clearDepth = 1.0f;
  ctx->clearStencil = 0;
  ctx->block32 = false;

  // Viewport.
  ctx->viewportX = 0;
  ctx->viewportY = 0;
  ctx->viewportW = 0;
  ctx->viewportH = 0;

  // Scissor.
  ctx->scissorMode = GPU_SCISSOR_DISABLE;
  ctx->scissorX = 0;
  ctx->scissorY = 0;
  ctx->scissorW = 0;
  ctx->scissorH = 0;

  // Program.
  ctx->currentProgram = GLASS_INVALID_OBJECT;

  // Attributes.
  for (size_t i = 0; i < GLASS_NUM_ATTRIB_REGS; i++) {
    AttributeInfo *attrib = &ctx->attribs[i];
    attrib->type = GL_FLOAT;
    attrib->count = 4;
    attrib->stride = 0;
    attrib->boundBuffer = 0;
    attrib->physAddr = 0;
    attrib->components[0] = 0.0f;
    attrib->components[1] = 0.0f;
    attrib->components[2] = 0.0f;
    attrib->components[3] = 1.0f;
  }

  for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; i++)
    ctx->attribSlots[i] = GLASS_NUM_ATTRIB_REGS;

  // Combiners.
  ctx->combinerStage = 0;
  for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; i++) {
    CombinerInfo *combiner = &ctx->combiners[i];
    combiner->rgbSrc[0] = !i ? GL_PRIMARY_COLOR : GL_PREVIOUS;
    combiner->rgbSrc[1] = GL_PRIMARY_COLOR;
    combiner->rgbSrc[2] = GL_PRIMARY_COLOR;
    combiner->alphaSrc[0] = !i ? GL_PRIMARY_COLOR : GL_PREVIOUS;
    combiner->alphaSrc[1] = GL_PRIMARY_COLOR;
    combiner->alphaSrc[2] = GL_PRIMARY_COLOR;
    combiner->rgbOp[0] = GL_SRC_COLOR;
    combiner->rgbOp[1] = GL_SRC_COLOR;
    combiner->rgbOp[2] = GL_SRC_COLOR;
    combiner->alphaOp[0] = GL_SRC_ALPHA;
    combiner->alphaOp[1] = GL_SRC_ALPHA;
    combiner->alphaOp[2] = GL_SRC_ALPHA;
    combiner->rgbFunc = GL_REPLACE;
    combiner->alphaFunc = GL_REPLACE;
    combiner->rgbScale = 1.0f;
    combiner->alphaScale = 1.0f;
    combiner->color = 0xFFFFFFFF;
  }

  // Fragment.
  ctx->fragMode = GL_FRAGOP_MODE_DEFAULT_PICA;
  ctx->blendMode = false;

  // Color and depth.
  ctx->writeRed = true;
  ctx->writeGreen = true;
  ctx->writeBlue = true;
  ctx->writeAlpha = true;
  ctx->writeDepth = true;
  ctx->depthTest = false;
  ctx->depthFunc = GL_LESS;

  // Depth map.
  ctx->depthNear = 0.0f;
  ctx->depthFar = 1.0f;
  ctx->polygonOffset = false;
  ctx->polygonFactor = 0.0f;
  ctx->polygonUnits = 0.0f;

  // Early depth.
  ctx->earlyDepthTest = false;
  ctx->clearEarlyDepth = 1.0f;
  ctx->earlyDepthFunc = GL_LESS;

  // Stencil.
  ctx->stencilTest = false;
  ctx->stencilFunc = GL_ALWAYS;
  ctx->stencilRef = 0;
  ctx->stencilMask = 0xFFFFFFFF;
  ctx->stencilWriteMask = 0xFFFFFFFF;
  ctx->stencilFail = GL_KEEP;
  ctx->stencilDepthFail = GL_KEEP;
  ctx->stencilPass = GL_KEEP;

  // Cull face.
  ctx->cullFace = false;
  ctx->cullFaceMode = GL_BACK;
  ctx->frontFaceMode = GL_CCW;

  // Alpha.
  ctx->alphaTest = false;
  ctx->alphaFunc = GL_ALWAYS;
  ctx->alphaRef = 0;

  // Blend.
  ctx->blendColor = 0;
  ctx->blendEqRGB = GL_FUNC_ADD;
  ctx->blendEqAlpha = GL_FUNC_ADD;
  ctx->blendSrcRGB = GL_ONE;
  ctx->blendDstRGB = GL_ZERO;
  ctx->blendSrcAlpha = GL_ONE;
  ctx->blendDstAlpha = GL_ZERO;

  // Logic Op.
  ctx->logicOp = GL_COPY;
}

void GLASS_context_finiContext(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  if (ctx == g_Context)
    BindContext(NULL);

  GPUFini(ctx);
}

void GLASS_context_bindContext(CtxImpl *ctx) {
  const bool skipUpdate = (ctx == g_Context) || (!g_Context && ctx == g_OldCtx);

  if (g_Context)
    GPUFlushQueue(g_Context, true);

  // Bind context.
  if (ctx != g_Context) {
    g_OldCtx = g_Context;
    g_Context = ctx;
  }

  // Run new queue.
  if (g_Context) {
    GPURunQueue(g_Context, true);

    if (!skipUpdate)
      g_Context->flags = CONTEXT_FLAG_ALL;
  }
}

CtxImpl *GLASS_context_getContext(void) {
  Assert(g_Context, "Context was nullptr!");
  return g_Context;
}

CtxImpl *GLASS_context_updateContext(void) {
  Assert(g_Context, "Context was nullptr!");
  GPUEnableRegs(g_Context);

  // Handle framebuffer.
  if (g_Context->flags & CONTEXT_FLAG_FRAMEBUFFER) {
    FramebufferInfo *info = (FramebufferInfo *)g_Context->framebuffer;

    // Flush buffers if required.
    if (g_Context->flags & CONTEXT_FLAG_DRAW) {
      FlushFramebuffer();

      if (g_Context->earlyDepthTest) {
        ClearEarlyDepthBuffer();
        g_Context->flags &= ~CONTEXT_FLAG_EARLY_DEPTH_CLEAR;
      }

      g_Context->flags &= ~CONTEXT_FLAG_DRAW;
    }

    BindFramebuffer(info, g_Context->block32);
    g_Context->flags &= ~CONTEXT_FLAG_FRAMEBUFFER;
  }

  // Handle draw.
  if (g_Context->flags & CONTEXT_FLAG_DRAW) {
    FlushFramebuffer();
    InvalidateFramebuffer();
    g_Context->flags &= ~CONTEXT_FLAG_DRAW;
  }

  // Handle viewport.
  if (g_Context->flags & CONTEXT_FLAG_VIEWPORT) {
    SetViewport(g_Context->viewportX, g_Context->viewportY,
                g_Context->viewportW, g_Context->viewportH);
    g_Context->flags &= ~CONTEXT_FLAG_VIEWPORT;
  }

  // Handle scissor.
  if (g_Context->flags & CONTEXT_FLAG_SCISSOR) {
    SetScissorTest(g_Context->scissorMode, g_Context->scissorX,
                   g_Context->scissorY, g_Context->scissorW,
                   g_Context->scissorH);
    g_Context->flags &= ~CONTEXT_FLAG_SCISSOR;
  }

  // Handle program.
  if (g_Context->flags & CONTEXT_FLAG_PROGRAM) {
    ProgramInfo *pinfo = (ProgramInfo *)g_Context->currentProgram;
    if (pinfo) {
      ShaderInfo *vs = NULL;
      ShaderInfo *gs = NULL;

      if (pinfo->flags & PROGRAM_FLAG_UPDATE_VERTEX) {
        vs = (ShaderInfo *)pinfo->linkedVertex;
        pinfo->flags &= ~PROGRAM_FLAG_UPDATE_VERTEX;
      }

      if (pinfo->flags & PROGRAM_FLAG_UPDATE_GEOMETRY) {
        gs = (ShaderInfo *)pinfo->linkedGeometry;
        pinfo->flags &= ~PROGRAM_FLAG_UPDATE_GEOMETRY;
      }

      BindShaders(vs, gs);

      if (vs)
        UploadConstUniforms(vs);

      if (gs)
        UploadConstUniforms(gs);
    }

    g_Context->flags &= ~CONTEXT_FLAG_PROGRAM;
  }

  // Handle uniforms.
  if (ObjectIsProgram(g_Context->currentProgram)) {
    ProgramInfo *pinfo = (ProgramInfo *)g_Context->currentProgram;
    ShaderInfo *vs = (ShaderInfo *)pinfo->linkedVertex;
    ShaderInfo *gs = (ShaderInfo *)pinfo->linkedGeometry;

    if (vs)
      UploadUniforms(vs);

    if (gs)
      UploadUniforms(gs);
  }

  // Handle attributes.
  if (g_Context->flags & CONTEXT_FLAG_ATTRIBS) {
    UploadAttributes(g_Context->attribs, g_Context->attribSlots);
    g_Context->flags &= ~CONTEXT_FLAG_ATTRIBS;
  }

  // Handle combiners.
  if (g_Context->flags & CONTEXT_FLAG_COMBINERS) {
    SetCombiners(g_Context->combiners);
    g_Context->flags &= ~CONTEXT_FLAG_COMBINERS;
  }

  // Handle fragment.
  if (g_Context->flags & CONTEXT_FLAG_FRAGMENT) {
    SetFragOp(g_Context->fragMode, g_Context->blendMode);
    g_Context->flags &= ~CONTEXT_FLAG_FRAGMENT;
  }

  // Handle color and depth masks.
  if (g_Context->flags & CONTEXT_FLAG_COLOR_DEPTH) {
    SetColorDepthMask(g_Context->writeRed, g_Context->writeGreen,
                      g_Context->writeBlue, g_Context->writeAlpha,
                      g_Context->writeDepth, g_Context->depthTest,
                      g_Context->depthFunc);
    // TODO: check gas!!!!
    g_Context->flags &= ~CONTEXT_FLAG_COLOR_DEPTH;
  }

  // Handle depth map.
  if (g_Context->flags & CONTEXT_FLAG_DEPTHMAP) {
    FramebufferInfo *fb = (FramebufferInfo *)g_Context->framebuffer;
    RenderbufferInfo *db = fb ? fb->depthBuffer : NULL;
    SetDepthMap(g_Context->polygonOffset, g_Context->depthNear,
                g_Context->depthFar,
                g_Context->polygonOffset ? g_Context->polygonUnits : 0.0f,
                db ? db->format : 0u);
    g_Context->flags &= ~CONTEXT_FLAG_DEPTHMAP;
  }

  // Handle early depth.
  if (g_Context->flags & CONTEXT_FLAG_EARLY_DEPTH) {
    SetEarlyDepthTest(g_Context->earlyDepthTest);
    if (g_Context->earlyDepthTest) {
      SetEarlyDepthFunc(g_Context->earlyDepthFunc);
      SetEarlyDepthClear(g_Context->clearEarlyDepth);
    }
    g_Context->flags &= ~CONTEXT_FLAG_EARLY_DEPTH;
  }

  // Handle early depth clear.
  if (g_Context->flags & CONTEXT_FLAG_EARLY_DEPTH_CLEAR) {
    if (g_Context->earlyDepthTest)
      ClearEarlyDepthBuffer();
    g_Context->flags &= ~CONTEXT_FLAG_EARLY_DEPTH_CLEAR;
  }

  // Handle stencil.
  if (g_Context->flags & CONTEXT_FLAG_STENCIL) {
    SetStencilTest(g_Context->stencilTest, g_Context->stencilFunc,
                   g_Context->stencilRef, g_Context->stencilMask,
                   g_Context->stencilWriteMask);
    if (g_Context->stencilTest) {
      SetStencilOp(g_Context->stencilFail, g_Context->stencilDepthFail,
                   g_Context->stencilPass);
    }
    g_Context->flags &= ~CONTEXT_FLAG_STENCIL;
  }

  // Handle cull face.
  if (g_Context->flags & CONTEXT_FLAG_CULL_FACE) {
    SetCullFace(g_Context->cullFace, g_Context->cullFaceMode,
                g_Context->frontFaceMode);
    g_Context->flags &= ~CONTEXT_FLAG_CULL_FACE;
  }

  // Handle alpha.
  if (g_Context->flags & CONTEXT_FLAG_ALPHA) {
    SetAlphaTest(g_Context->alphaTest, g_Context->alphaFunc,
                 g_Context->alphaRef);
    g_Context->flags &= ~CONTEXT_FLAG_ALPHA;
  }

  // Handle blend & logic op.
  if (g_Context->flags & CONTEXT_FLAG_BLEND) {
    if (g_Context->blendMode) {
      SetBlendFunc(g_Context->blendEqRGB, g_Context->blendEqAlpha,
                   g_Context->blendSrcRGB, g_Context->blendDstRGB,
                   g_Context->blendSrcAlpha, g_Context->blendDstAlpha);
      SetBlendColor(g_Context->blendColor);
    } else {
      SetLogicOp(g_Context->logicOp);
    }
    g_Context->flags &= ~CONTEXT_FLAG_BLEND;
  }

  GPUDisableRegs(g_Context);
  return g_Context;
}

void GLASS_context_setError(const GLenum error) {
  Assert(g_Context, "Context was nullptr!");
  if (g_Context->lastError == GL_NO_ERROR)
    g_Context->lastError = error;
}