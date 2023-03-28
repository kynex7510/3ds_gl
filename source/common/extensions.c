#include "context.h"
#include "utility.h"

#define IsBlockMode(mode)                                                      \
  (((mode) == GL_BLOCK8_PICA) || ((mode) == GL_BLOCK32_PICA))

#define IsCombinerFunc(func)                                                   \
  (((func) == GL_REPLACE) || ((func) == GL_MODULATE) || ((func) == GL_ADD) ||  \
   (func) == GL_ADD_SIGNED || (func) == GL_INTERPOLATE ||                      \
   (func) == GL_SUBTRACT || (func) == GL_DOT3_RGB || (func) == GL_DOT3_RGBA || \
   (func) == GL_MULT_ADD_PICA || (func) == GL_ADD_MULT_PICA)

#define IsCombinerSrc(src)                                                     \
  (((src) == GL_PRIMARY_COLOR) || ((src) == GL_FRAGMENT_PRIMARY_COLOR_PICA) || \
   ((src) == GL_FRAGMENT_SECONDARY_COLOR_PICA) || ((src) == GL_TEXTURE0) ||    \
   ((src) == GL_TEXTURE1) || ((src) == GL_TEXTURE2) ||                         \
   ((src) == GL_TEXTURE3) || ((src) == GL_PREVIOUS_BUFFER_PICA) ||             \
   ((src) == GL_CONSTANT) || ((src) == GL_PREVIOUS))

#define IsCombinerOpAlpha(op)                                                  \
  (((op) == GL_SRC_ALPHA) || ((op) == GL_ONE_MINUS_SRC_ALPHA) ||               \
   ((op) == GL_SRC_R_PICA) || ((op) == GL_ONE_MINUS_SRC_R_PICA) ||             \
   ((op) == GL_SRC_G_PICA) || ((op) == GL_ONE_MINUS_SRC_G_PICA) ||             \
   ((op) == GL_SRC_B_PICA) || ((op) == GL_ONE_MINUS_SRC_B_PICA))

#define IsCombinerOpRGB(op)                                                    \
  (IsCombinerOpAlpha((op)) || ((op) == GL_SRC_COLOR) ||                        \
   ((op) == GL_ONE_MINUS_SRC_COLOR))

#define IsCombinerScale(scale)                                                 \
  (((scale) == 1.0f) || ((scale) == 2.0f) || ((scale) == 4.0f))

#define IsEarlyDepthFunc(func)                                                 \
  (((func) == GL_LESS) || ((func) == GL_LEQUAL) || ((func) == GL_GREATER) ||   \
   ((func) != GL_GEQUAL))

#define IsFragOp(op)                                                           \
  (((mode) != GL_FRAGOP_MODE_DEFAULT_PICA) ||                                  \
   ((mode) == GL_FRAGOP_MODE_SHADOW_PICA) ||                                   \
   ((mode) == GL_FRAGOP_MODE_GAS_PICA))

// Extensions

void glBlockModePICA(GLenum mode) {
  if (!IsBlockMode(mode)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();

  // Set block mode.
  const bool old = ctx->block32;
  ctx->block32 = mode == GL_BLOCK32_PICA;
  if (old != ctx->block32)
    ctx->flags |= CONTEXT_FLAG_FRAMEBUFFER;

  // Early depth is only available in mode 32.
  if (!ctx->block32 && ctx->earlyDepthTest) {
    ctx->earlyDepthTest = false;
    ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH;
  }
}

void glClearEarlyDepthPICA(GLclampf depth) {
  depth = GLClampFloat(depth);
  CtxImpl *ctx = GetContext();
  if (ctx->clearEarlyDepth != depth) {
    ctx->clearEarlyDepth = depth;
    if (ctx->earlyDepthTest)
      ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH;
  }
}

void glCombinerColorPICA(GLclampf red, GLclampf green, GLclampf blue,
                         GLclampf alpha) {
  CtxImpl *ctx = GetContext();
  CombinerInfo *combiner = &ctx->combiners[ctx->combinerStage];

  u32 color = (u32)(0xFF * GLClampFloat(red)) << 24;
  color |= (u32)(0xFF * GLClampFloat(green)) << 16;
  color |= (u32)(0xFF * GLClampFloat(blue)) << 8;
  color |= (u32)(0xFF * GLClampFloat(alpha));

  if (combiner->color != color) {
    combiner->color = color;
    ctx->flags |= CONTEXT_FLAG_COMBINERS;
  }
}

void glCombinerFuncPICA(GLenum pname, GLenum func) {
  if (!IsCombinerFunc(func)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();
  CombinerInfo *combiner = &ctx->combiners[ctx->combinerStage];

  switch (pname) {
  case GL_COMBINE_RGB:
    if (combiner->rgbFunc != func) {
      combiner->rgbFunc = func;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }

    // Special case.
    if (func == GL_DOT3_RGBA && combiner->alphaFunc != GL_DOT3_RGBA) {
      combiner->alphaFunc = GL_DOT3_RGBA;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  case GL_COMBINE_ALPHA:
    if (combiner->alphaFunc != func) {
      combiner->alphaFunc = func;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }

    // Special case.
    if (func == GL_DOT3_RGBA && combiner->rgbFunc != GL_DOT3_RGBA) {
      combiner->rgbFunc = GL_DOT3_RGBA;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  default:
    SetError(GL_INVALID_ENUM);
  }
}

void glCombinerOpPICA(GLenum pname, GLenum op) {
  CtxImpl *ctx = GetContext();
  CombinerInfo *combiner = &ctx->combiners[ctx->combinerStage];
  size_t index = 2;

  switch (pname) {
  case GL_OPERAND0_RGB:
    index -= 1;
  case GL_OPERAND1_RGB:
    index -= 1;
  case GL_OPERAND2_RGB:
    if (!IsCombinerOpRGB(op)) {
      SetError(GL_INVALID_ENUM);
      return;
    }

    if (combiner->rgbOp[index] != op) {
      combiner->rgbOp[index] = op;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  case GL_OPERAND0_ALPHA:
    index -= 1;
  case GL_OPERAND1_ALPHA:
    index -= 1;
  case GL_OPERAND2_ALPHA:
    if (!IsCombinerOpAlpha(op)) {
      SetError(GL_INVALID_ENUM);
      return;
    }

    if (combiner->alphaOp[index] != op) {
      combiner->alphaOp[index] = op;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  default:
    SetError(GL_INVALID_ENUM);
  }
}

void glCombinerScalePICA(GLenum pname, GLfloat scale) {
  if (!IsCombinerScale(scale)) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();
  CombinerInfo *combiner = &ctx->combiners[ctx->combinerStage];

  switch (pname) {
  case GL_RGB_SCALE:
    if (combiner->rgbScale != scale) {
      combiner->rgbScale = scale;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  case GL_ALPHA_SCALE:
    if (combiner->alphaScale != scale) {
      combiner->alphaScale = scale;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return;
  }
}

void glCombinerSrcPICA(GLenum pname, GLenum src) {
  if (!IsCombinerSrc(src)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();
  CombinerInfo *combiner = &ctx->combiners[ctx->combinerStage];
  size_t index = 2;

  switch (pname) {
  case GL_SRC0_RGB:
    index -= 1;
  case GL_SRC1_RGB:
    index -= 1;
  case GL_SRC2_RGB:
    if (combiner->rgbSrc[index] != src) {
      combiner->rgbSrc[index] = src;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  case GL_SRC0_ALPHA:
    index -= 1;
  case GL_SRC1_ALPHA:
    index -= 1;
  case GL_SRC2_ALPHA:
    if (combiner->alphaSrc[index] != src) {
      combiner->alphaSrc[index] = src;
      ctx->flags |= CONTEXT_FLAG_COMBINERS;
    }
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return;
  }
}

void glCombinerStagePICA(GLint index) {
  if (index < 0 || index >= GLASS_NUM_COMBINER_STAGES) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  GetContext()->combinerStage = index;
}

void glEarlyDepthFuncPICA(GLenum func) {
  if (!IsEarlyDepthFunc(func)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();
  if (ctx->earlyDepthFunc != func) {
    ctx->earlyDepthFunc = func;
    if (ctx->earlyDepthTest)
      ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH;
  }
}

void glFragOpPICA(GLenum mode) {
  if ((mode != GL_FRAGOP_MODE_DEFAULT_PICA) &&
      (mode != GL_FRAGOP_MODE_SHADOW_PICA) &&
      (mode != GL_FRAGOP_MODE_GAS_PICA)) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  CtxImpl *ctx = GetContext();
  if (ctx->fragMode != mode) {
    ctx->fragMode = mode;
    ctx->flags |= CONTEXT_FLAG_FRAGMENT;
  }
}