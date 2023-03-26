#include "get.h"
#include "context.h"
#include "utility.h"

#include <math.h>

#define BOOL_HANDLER(name)                                                     \
  static void GLASS_get_##name(const CtxImpl *ctx, GLboolean *params)

#define BOOL_ENTRY(name, size)                                                 \
  { (name), (size), GLASS_get_##name }

#define FLOAT_HANDLER(name)                                                    \
  static void GLASS_get_##name(const CtxImpl *ctx, GLfloat *params)

#define FLOAT_ENTRY(name, size)                                                \
  { (name), (size), GLASS_get_##name }

#define INT_HANDLER(name)                                                      \
  static void GLASS_get_##name(const CtxImpl *ctx, GLint *params)

#define INT_ENTRY(name, size)                                                  \
  { (name), (size), GLASS_get_##name }

// Types

typedef void (*BoolHandler)(const CtxImpl *ctx, GLboolean *params);
typedef void (*FloatHandler)(const CtxImpl *ctx, GLfloat *params);
typedef void (*IntHandler)(const CtxImpl *ctx, GLint *params);

typedef struct {
  const GLenum pname;
  const size_t numParams;
  BoolHandler getBools;
} BoolEntry;

typedef struct {
  const GLenum pname;
  const size_t numParams;
  FloatHandler getFloats;
} FloatEntry;

typedef struct {
  const GLenum pname;
  const size_t numParams;
  IntHandler getInts;
} IntEntry;

// Bool entries

BOOL_HANDLER(GL_BLEND) { params[0] = ctx->blendMode ? GL_TRUE : GL_FALSE; }

BOOL_HANDLER(GL_COLOR_WRITEMASK) {
  params[0] = ctx->writeRed ? GL_TRUE : GL_FALSE;
  params[1] = ctx->writeGreen ? GL_TRUE : GL_FALSE;
  params[2] = ctx->writeBlue ? GL_TRUE : GL_FALSE;
  params[3] = ctx->writeAlpha ? GL_TRUE : GL_FALSE;
}

BOOL_HANDLER(GL_CULL_FACE) { params[0] = ctx->cullFace ? GL_TRUE : GL_FALSE; }
BOOL_HANDLER(GL_DEPTH_TEST) { params[0] = ctx->depthTest ? GL_TRUE : GL_FALSE; }

BOOL_HANDLER(GL_DEPTH_WRITEMASK) {
  params[0] = ctx->writeDepth ? GL_TRUE : GL_FALSE;
}

BOOL_HANDLER(GL_DITHER) { params[0] = GL_FALSE; }

static const BoolEntry g_BoolEntries[] = {
    BOOL_ENTRY(GL_BLEND, 1),           BOOL_ENTRY(GL_COLOR_WRITEMASK, 4),
    BOOL_ENTRY(GL_CULL_FACE, 1),       BOOL_ENTRY(GL_DEPTH_TEST, 1),
    BOOL_ENTRY(GL_DEPTH_WRITEMASK, 1), BOOL_ENTRY(GL_DITHER, 1),
};

// Float entries

FLOAT_HANDLER(GL_ALIASED_LINE_WIDTH_RANGE) {
  params[0] = 1.0f;
  params[1] = 1.0f;
}

FLOAT_HANDLER(GL_ALIASED_POINT_SIZE_RANGE) {
  params[0] = 1.0f;
  params[1] = 1.0f;
}

FLOAT_HANDLER(GL_BLEND_COLOR) {
  params[0] = (ctx->blendColor >> 24) & 0xFF;
  params[1] = (ctx->blendColor >> 16) & 0xFF;
  params[2] = (ctx->blendColor >> 8) & 0xFF;
  params[3] = ctx->blendColor & 0xFF;
}

FLOAT_HANDLER(GL_COLOR_CLEAR_VALUE) {
  params[0] = (ctx->clearColor >> 24) & 0xFF;
  params[1] = (ctx->clearColor >> 16) & 0xFF;
  params[2] = (ctx->clearColor >> 8) & 0xFF;
  params[3] = ctx->clearColor & 0xFF;
}

FLOAT_HANDLER(GL_DEPTH_CLEAR_VALUE) { params[0] = ctx->clearDepth; }

FLOAT_HANDLER(GL_DEPTH_RANGE) {
  params[0] = ctx->depthNear;
  params[1] = ctx->depthFar;
}

FLOAT_HANDLER(GL_POLYGON_OFFSET_FACTOR) { params[0] = ctx->polygonFactor; }
FLOAT_HANDLER(GL_POLYGON_OFFSET_UNITS) { params[0] = ctx->polygonUnits; }

static const FloatEntry g_FloatEntries[] = {
    FLOAT_ENTRY(GL_ALIASED_LINE_WIDTH_RANGE, 2),
    FLOAT_ENTRY(GL_ALIASED_POINT_SIZE_RANGE, 2),
    FLOAT_ENTRY(GL_BLEND_COLOR, 4),
    FLOAT_ENTRY(GL_COLOR_CLEAR_VALUE, 4),
    FLOAT_ENTRY(GL_DEPTH_CLEAR_VALUE, 1),
    FLOAT_ENTRY(GL_DEPTH_RANGE, 2),
    FLOAT_ENTRY(GL_POLYGON_OFFSET_FACTOR, 1),
    FLOAT_ENTRY(GL_POLYGON_OFFSET_UNITS, 1),
};

// Integer entries

INT_HANDLER(GL_ACTIVE_TEXTURE) { Unreachable("Unimplemented!"); }
INT_HANDLER(GL_ALPHA_BITS) { Unreachable("Unimplemented!"); }
INT_HANDLER(GL_ARRAY_BUFFER_BINDING) { params[0] = ctx->arrayBuffer; }
INT_HANDLER(GL_BLEND_DST_ALPHA) { params[0] = ctx->blendDstAlpha; }
INT_HANDLER(GL_BLEND_DST_RGB) { params[0] = ctx->blendDstRGB; }
INT_HANDLER(GL_BLEND_EQUATION_ALPHA) { params[0] = ctx->blendEqAlpha; }
INT_HANDLER(GL_BLEND_EQUATION_RGB) { params[0] = ctx->blendEqRGB; }
INT_HANDLER(GL_BLEND_SRC_ALPHA) { params[0] = ctx->blendSrcAlpha; }
INT_HANDLER(GL_BLEND_SRC_RGB) { params[0] = ctx->blendSrcRGB; }
INT_HANDLER(GL_BLUE_BITS) { Unreachable("Unimplemented!"); }
INT_HANDLER(GL_COMPRESSED_TEXTURE_FORMATS) { Unreachable("Unimplemented!"); }
INT_HANDLER(GL_CULL_FACE_MODE) { params[0] = ctx->cullFaceMode; }
INT_HANDLER(GL_CURRENT_PROGRAM) { params[0] = ctx->currentProgram; }
INT_HANDLER(GL_DEPTH_BITS) { Unreachable("Unimplemented!"); }
INT_HANDLER(GL_DEPTH_FUNC) { params[0] = ctx->depthFunc; }

INT_HANDLER(GL_ELEMENT_ARRAY_BUFFER_BINDING) {
  params[0] = ctx->elementArrayBuffer;
}

INT_HANDLER(GL_FRAMEBUFFER_BINDING) { params[0] = ctx->framebuffer; }
INT_HANDLER(GL_FRONT_FACE) { params[0] = ctx->frontFaceMode; }

INT_HANDLER(GL_GREEN_BITS) { Unreachable("Unimplemented!"); }

INT_HANDLER(GL_IMPLEMENTATION_COLOR_READ_FORMAT) {
  Unreachable("Unimplemented!");
}

INT_HANDLER(GL_IMPLEMENTATION_COLOR_READ_TYPE) {
  Unreachable("Unimplemented!");
}

static const IntEntry g_IntEntries[] = {
    INT_ENTRY(GL_ACTIVE_TEXTURE, GLASS_GET_FAILED),
    INT_ENTRY(GL_ALPHA_BITS, GLASS_GET_FAILED),
    INT_ENTRY(GL_ARRAY_BUFFER_BINDING, 1),
    INT_ENTRY(GL_BLEND_DST_ALPHA, 1),
    INT_ENTRY(GL_BLEND_DST_RGB, 1),
    INT_ENTRY(GL_BLEND_EQUATION_ALPHA, 1),
    INT_ENTRY(GL_BLEND_EQUATION_RGB, 1),
    INT_ENTRY(GL_BLEND_SRC_ALPHA, 1),
    INT_ENTRY(GL_BLEND_SRC_RGB, 1),
    INT_ENTRY(GL_BLUE_BITS, GLASS_GET_FAILED),
    INT_ENTRY(GL_COMPRESSED_TEXTURE_FORMATS, GLASS_GET_FAILED),
    INT_ENTRY(GL_CULL_FACE_MODE, 1),
    INT_ENTRY(GL_CURRENT_PROGRAM, 1),
    INT_ENTRY(GL_DEPTH_BITS, GLASS_GET_FAILED),
    INT_ENTRY(GL_DEPTH_FUNC, 1),
    INT_ENTRY(GL_ELEMENT_ARRAY_BUFFER_BINDING, 1),
    INT_ENTRY(GL_FRAMEBUFFER_BINDING, 1),
    INT_ENTRY(GL_FRONT_FACE, 1),
    INT_ENTRY(GL_GREEN_BITS, GLASS_GET_FAILED),
    INT_ENTRY(GL_IMPLEMENTATION_COLOR_READ_FORMAT, GLASS_GET_FAILED),
    INT_ENTRY(GL_IMPLEMENTATION_COLOR_READ_TYPE, GLASS_GET_FAILED),
};

// Get

size_t GLASS_get_getBools(const GLenum pname, GLboolean *params) {
  for (size_t i = 0; i < sizeof(g_BoolEntries); i++) {
    const BoolEntry *entry = &g_BoolEntries[i];
    if (entry->pname == pname) {
      entry->getBools(GetContext(), params);
      return entry->numParams;
    }
  }

  return GLASS_GET_FAILED;
}

size_t GLASS_get_getFloats(const GLenum pname, GLfloat *params) {
  for (size_t i = 0; i < sizeof(g_FloatEntries); i++) {
    const FloatEntry *entry = &g_FloatEntries[i];
    if (entry->pname == pname) {
      entry->getFloats(GetContext(), params);
      return entry->numParams;
    }
  }

  return GLASS_GET_FAILED;
}

size_t GLASS_get_getInts(const GLenum pname, GLint *params) {
  for (size_t i = 0; i < sizeof(g_IntEntries); i++) {
    const IntEntry *entry = &g_IntEntries[i];
    if (entry->pname == pname) {
      entry->getInts(GetContext(), params);
      return entry->numParams;
    }
  }

  return GLASS_GET_FAILED;
}

// TODO: colors and normals must be remapped from [-1.0, 1.0]
GLint GLASS_get_castFloatAsInt(const GLenum pname, const GLfloat value,
                               const size_t index) {
  return (GLint)round(value);
}