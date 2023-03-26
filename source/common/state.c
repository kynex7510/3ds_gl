#include "context.h"
#include "get.h"
#include "utility.h"

// Globals

static const char *GLASS_INFO_VENDOR = "Kynex7510";
static const char *GLASS_INFO_RENDERER = "PICA200";
static const char *GLASS_INFO_VERSION = "OpenGL ES 2.0";
static const char *GLASS_INFO_SHADING_LANGUAGE_VERSION = "SHBIN 1.0";
static const char *GLASS_INFO_EXTENSIONS = ""; // TODO

// Helpers

#define SetCapability GLASS_state_setCapability
static void GLASS_state_setCapability(const GLenum cap, const bool enabled) {
  CtxImpl *ctx = GetContext();

  switch (cap) {
  case GL_ALPHA_TEST:
    ctx->alphaTest = enabled;
    ctx->flags |= CONTEXT_FLAG_ALPHA;
    return;
  case GL_BLEND:
  case GL_COLOR_LOGIC_OP:
    ctx->blendMode = (enabled ? (cap == GL_BLEND) : (cap == GL_COLOR_LOGIC_OP));
    ctx->flags |= (CONTEXT_FLAG_FRAGMENT | CONTEXT_FLAG_BLEND);
    return;
  case GL_CULL_FACE:
    ctx->cullFace = enabled;
    ctx->flags |= CONTEXT_FLAG_CULL_FACE;
    return;
  case GL_DEPTH_TEST:
    ctx->depthTest = enabled;
    ctx->flags |= CONTEXT_FLAG_COLOR_DEPTH;
    return;
  case GL_EARLY_DEPTH_TEST_PICA:
    if (ctx->block32) {
      ctx->earlyDepthTest = enabled;
      ctx->flags |= CONTEXT_FLAG_EARLY_DEPTH;
    } else {
      SetError(GL_INVALID_OPERATION);
    }
    return;
  case GL_POLYGON_OFFSET_FILL:
    ctx->polygonOffset = enabled;
    ctx->flags |= CONTEXT_FLAG_DEPTHMAP;
    return;
  case GL_SCISSOR_TEST:
    ctx->scissorMode = (enabled ? GPU_SCISSOR_NORMAL : GPU_SCISSOR_DISABLE);
    ctx->flags |= CONTEXT_FLAG_SCISSOR;
    return;
  case GL_SCISSOR_TEST_INVERTED_PICA:
    ctx->scissorMode = (enabled ? GPU_SCISSOR_INVERT : GPU_SCISSOR_DISABLE);
    ctx->flags |= CONTEXT_FLAG_SCISSOR;
    return;
  case GL_STENCIL_TEST:
    ctx->stencilTest = enabled;
    ctx->flags |= CONTEXT_FLAG_STENCIL;
    return;
  case GL_DEPTH_STENCIL_COPY_PICA: // TODO
  }

  SetError(GL_INVALID_ENUM);
}

// State

void glDisable(GLenum cap) { SetCapability(cap, false); }
void glEnable(GLenum cap) { SetCapability(cap, true); }

GLboolean glIsEnabled(GLenum cap) {
  CtxImpl *ctx = GetContext();

  switch (cap) {
  case GL_ALPHA_TEST:
    return ctx->alphaTest ? GL_TRUE : GL_FALSE;
  case GL_BLEND:
    return ctx->blendMode ? GL_TRUE : GL_FALSE;
  case GL_COLOR_LOGIC_OP:
    return !ctx->blendMode ? GL_TRUE : GL_FALSE;
  case GL_CULL_FACE:
    return ctx->cullFace ? GL_TRUE : GL_FALSE;
  case GL_DEPTH_TEST:
    return ctx->depthTest ? GL_TRUE : GL_FALSE;
  case GL_EARLY_DEPTH_TEST_PICA:
    return ctx->earlyDepthTest ? GL_TRUE : GL_FALSE;
  case GL_POLYGON_OFFSET_FILL:
    return ctx->polygonOffset ? GL_TRUE : GL_FALSE;
  case GL_SCISSOR_TEST:
    return ctx->scissorMode == GPU_SCISSOR_NORMAL ? GL_TRUE : GL_FALSE;
  case GL_SCISSOR_TEST_INVERTED_PICA:
    return ctx->scissorMode == GPU_SCISSOR_INVERT ? GL_TRUE : GL_FALSE;
  case GL_STENCIL_TEST:
    return ctx->stencilTest ? GL_TRUE : GL_FALSE;

  case GL_DEPTH_STENCIL_COPY_PICA: // TODO
  default:
    SetError(GL_INVALID_ENUM);
    return 0;
  }
}

GLenum glGetError(void) {
  CtxImpl *ctx = GetContext();
  GLenum error = ctx->lastError;
  ctx->lastError = GL_NO_ERROR;
  return error;
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
  Assert(params, "Params was nullptr!");

  // Try bools.
  if (GetBools(pname, params) != GLASS_GET_FAILED)
    return;

  // Try floats.
  GLfloat floatParams[GLASS_GET_MAX_PARAMS];
  size_t size = GetFloats(pname, floatParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = (floatParams[i] == 0.0 ? GL_FALSE : GL_TRUE);
    return;
  }

  // Try integers.
  GLint intParams[GLASS_GET_MAX_PARAMS];
  size = GetInts(pname, intParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = (intParams[i] == 0 ? GL_FALSE : GL_TRUE);
    return;
  }

  SetError(GL_INVALID_ENUM);
}

void glGetFloatv(GLenum pname, GLfloat *params) {
  Assert(params, "Params was nullptr!");

  // Try floats.
  if (GetFloats(pname, params) != GLASS_GET_FAILED)
    return;

  // Try booleans.
  GLboolean boolParams[GLASS_GET_MAX_PARAMS];
  size_t size = GetBools(pname, boolParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = (GLfloat)boolParams[i];
    return;
  }

  // Try integers.
  GLint intParams[GLASS_GET_MAX_PARAMS];
  size = GetInts(pname, intParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = (GLfloat)intParams[i];
    return;
  }

  SetError(GL_INVALID_ENUM);
}

void glGetIntegerv(GLenum pname, GLint *params) {
  Assert(params, "Params was nullptr!");

  // Try integers.
  if (GetInts(pname, params) != GLASS_GET_FAILED)
    return;

  // Try booleans.
  GLboolean boolParams[GLASS_GET_MAX_PARAMS];
  size_t size = GetBools(pname, boolParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = (GLint)boolParams[i];
    return;
  }

  // Try floats.
  GLfloat floatParams[GLASS_GET_MAX_PARAMS];
  size = GetFloats(pname, floatParams);
  if (size != GLASS_GET_FAILED) {
    for (size_t i = 0; i < size; i++)
      params[i] = CastFloatAsInt(pname, floatParams[i], i);
    return;
  }

  SetError(GL_INVALID_ENUM);
}

const GLubyte *glGetString(GLenum name) {
  switch (name) {
  case GL_VENDOR:
    return (const GLubyte *)GLASS_INFO_VENDOR;
  case GL_RENDERER:
    return (const GLubyte *)GLASS_INFO_RENDERER;
  case GL_VERSION:
    return (const GLubyte *)GLASS_INFO_VERSION;
  case GL_SHADING_LANGUAGE_VERSION:
    return (const GLubyte *)GLASS_INFO_SHADING_LANGUAGE_VERSION;
  case GL_EXTENSIONS:
    return (const GLubyte *)GLASS_INFO_EXTENSIONS;
  }

  SetError(GL_INVALID_ENUM);
  return NULL;
}