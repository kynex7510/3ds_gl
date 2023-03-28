#include "utility.h"

#include <string.h>

// Globals

extern u32 __ctru_linear_heap;

// Helpers

/*
#define UnpackIntVector GLASS_utility_unpackIntVector
static void GLASS_utility_unpackIntVector(const u32 in, u32 *out) {
  out[0] = in & 0xFF;
  out[1] = (in >> 8) & 0xFF;
  out[2] = (in >> 16) & 0xFF;
  out[3] = (in >> 24) & 0xFF;
}
*/

/*
#define UnpackFloatVector GLASS_utility_unpackFloatVector
static void GLASS_utility_unpackFloatVector(const u32 *in, float *out) {
  out[0] = f24tof32(in[2] >> 8);
  out[1] = f24tof32(((in[2] & 0xFF) << 16) | (in[1] >> 16));
  out[2] = f24tof32(((in[1] & 0xFFFF) << 8) | (in[0] >> 24));
  out[3] = f24tof32(in[0] & 0xFFFFFF);
}
*/

#define GetGXControl GLASS_utility_getGXControl
static u16 GLASS_utility_getGXControl(const bool start, const bool finished,
                                      const GLenum format) {
  const u16 fillWidth = GetFBPixelSize(format);
  return (u16)start | ((u16)finished << 1) | (fillWidth << 8);
}

// Utility

void GLASS_utility_log(const char *msg) {
#ifndef NDEBUG
  svcOutputDebugString(msg, strlen(msg));
#endif
}

void GLASS_utility_unreachable(const char *msg) {
  Log(msg);
  svcBreak(USERBREAK_PANIC);
  __builtin_unreachable();
}

void *GLASS_utility_convertPhysToVirt(const u32 addr) {
#define CONVERT_REGION(_name)                                                  \
  if (addr >= OS_##_name##_PADDR &&                                            \
      addr < (OS_##_name##_PADDR + OS_##_name##_SIZE))                         \
    return (void *)(addr - (OS_##_name##_PADDR + OS_##_name##_VADDR));

  CONVERT_REGION(FCRAM);
  CONVERT_REGION(VRAM);
  CONVERT_REGION(OLD_FCRAM);
  CONVERT_REGION(DSPRAM);
  CONVERT_REGION(QTMRAM);
  CONVERT_REGION(MMIO);

#undef CONVERT_REGION
  return NULL;
}

u32 GLASS_utility_getLinearBase(void) {
  return osConvertVirtToPhys((void *)__ctru_linear_heap);
}

float GLASS_utility_f24tof32(const u32 f) {
  union {
    float val;
    u32 bits;
  } cast;

  const u32 sign = f >> 23;

  if (!(f & 0x7FFFFF)) {
    cast.bits = (sign << 31);
  } else if (((f >> 16) & 0xFF) == 0x7F) {
    cast.bits = (sign << 31) | (0xFF << 23);
  } else {
    const u32 mantissa = f & 0xFFFF;
    const s32 exponent = ((f >> 16) & 0x7F) + 64;
    cast.bits = (sign << 31) | (exponent << 23) | (mantissa << 7);
  }

  return cast.val;
}

u32 GLASS_utility_convertRGBA8(const GLenum format, const u32 color) {
  u32 cvt = 0;

  switch (format) {
  case GL_RGBA8_OES:
    cvt = color;
    break;
  case GL_RGB8_OES:
    cvt = color >> 8;
    break;
  case GL_RGBA4:
    cvt = ((color >> 24) & 0xF) << 12;
    cvt |= ((color >> 16) & 0xF) << 8;
    cvt |= ((color >> 8) & 0xF) << 4;
    cvt |= color & 0xF;
    break;
  case GL_RGB5_A1:
    cvt = ((color >> 24) & 0x1F) << 11;
    cvt |= ((color >> 16) & 0x1F) << 6;
    cvt |= ((color >> 8) & 0x1F) << 1;
    cvt |= (color & 0xFF) != 0;
    break;
  case GL_RGB565:
    cvt = ((color >> 24) & 0x1F) << 11;
    cvt |= ((color >> 16) & 0x3F) << 5;
    cvt |= (color >> 8) & 0x1F;
    break;
  default:
    Unreachable("Invalid format!");
  }

  return cvt;
}

u32 GLASS_utility_getClearDepth(const GLenum format, const GLclampf factor,
                                const u8 stencil) {
  u32 clearDepth = 0;
  Assert(factor >= 0.0 && factor <= 1.0, "Invalid factor!");
  switch (format) {
  case GL_DEPTH_COMPONENT16:
    clearDepth = (u32)(0xFFFF * factor);
    break;
  case GL_DEPTH_COMPONENT24_OES:
    clearDepth = (u32)(0xFFFFFF * factor);
    break;
  case GL_DEPTH24_STENCIL8_EXT:
    clearDepth = (((u32)(0xFFFFFF * factor) << 8) | stencil);
    break;
  default:
    Unreachable("Invalid format!");
  }

  return clearDepth;
}

float GLASS_utility_GLClampFloat(const float f) {
  return f < 0.0 ? 0.0 : (f > 1.0 ? 1.0 : f);
}

size_t GLASS_utility_getFBFormatBytes(const GLenum format) {
  switch (format) {
  case GL_RGBA8_OES:
  case GL_DEPTH24_STENCIL8_EXT:
    return 4;
  case GL_RGB8_OES:
  case GL_DEPTH_COMPONENT24_OES:
    return 3;
  case GL_RGB5_A1:
  case GL_RGB565:
  case GL_RGBA4:
  case GL_DEPTH_COMPONENT16:
    return 2;
  }

  Unreachable("Invalid framebuffer format!");
}

size_t GLASS_utility_getFBPixelSize(const GLenum format) {
  return GetFBFormatBytes(format) - 2;
}

GLenum GLASS_utility_GSPToGLFBFormat(const GSPGPU_FramebufferFormat format) {
  switch (format) {
  case GSP_RGBA8_OES:
    return GL_RGBA8_OES;
  case GSP_BGR8_OES:
    return GL_RGB8_OES;
  case GSP_RGB565_OES:
    return GL_RGB565;
  case GSP_RGB5_A1_OES:
    return GL_RGB5_A1;
  case GSP_RGBA4_OES:
    return GL_RGBA4;
  }

  Unreachable("Invalid GSP format!");
}

GX_TRANSFER_FORMAT GLASS_utility_GLToGXFBFormat(const GLenum format) {
  switch (format) {
  case GL_RGBA8_OES:
    return GX_TRANSFER_FMT_RGBA8;
  case GL_RGB8_OES:
    return GX_TRANSFER_FMT_RGB8;
  case GL_RGB565:
    return GX_TRANSFER_FMT_RGB565;
  case GL_RGB5_A1:
    return GX_TRANSFER_FMT_RGB5A1;
  case GL_RGBA4:
    return GX_TRANSFER_FMT_RGBA4;
  }

  Unreachable("Invalid framebuffer format!");
}

GPU_COLORBUF GLASS_utility_GLToGPUFBFormat(const GLenum format) {
  switch (format) {
  case GL_RGBA8_OES:
    return GPU_RB_RGBA8;
  case GL_RGB8_OES:
    return GPU_RB_RGB8;
  case GL_RGB5_A1:
    return GPU_RB_RGBA5551;
  case GL_RGB565:
    return GPU_RB_RGB565;
  case GL_RGBA4:
    return GPU_RB_RGBA4;
  case GL_DEPTH_COMPONENT16:
    return GPU_RB_DEPTH16;
  case GL_DEPTH_COMPONENT24_OES:
    return GPU_RB_DEPTH24;
  case GL_DEPTH24_STENCIL8_EXT:
    return GPU_RB_DEPTH24_STENCIL8;
  }

  Unreachable("Invalid framebuffer format!");
}

GPU_FORMATS GLASS_utility_GLToGPUAttribType(const GLenum type) {
  switch (type) {
  case GL_BYTE:
    return GPU_BYTE;
  case GL_UNSIGNED_BYTE:
    return GPU_UNSIGNED_BYTE;
  case GL_SHORT:
    return GPU_SHORT;
  case GL_FLOAT:
    return GPU_FLOAT;
  }

  Unreachable("Invalid attribute type!");
}

GPU_TESTFUNC GLASS_utility_GLToGPUTestFunc(const GLenum func) {
  switch (func) {
  case GL_NEVER:
    return GPU_NEVER;
  case GL_LESS:
    return GPU_LESS;
  case GL_EQUAL:
    return GPU_EQUAL;
  case GL_LEQUAL:
    return GPU_LEQUAL;
  case GL_GREATER:
    return GPU_GREATER;
  case GL_NOTEQUAL:
    return GPU_NOTEQUAL;
  case GL_GEQUAL:
    return GPU_GEQUAL;
  case GL_ALWAYS:
    return GPU_ALWAYS;
  }

  Unreachable("Invalid test function!");
}

GPU_EARLYDEPTHFUNC GLASS_utility_GLToGPUEarlyDepthFunc(const GLenum func) {
  switch (func) {
  case GL_LESS:
    return GPU_EARLYDEPTH_LESS;
  case GL_LEQUAL:
    return GPU_EARLYDEPTH_LEQUAL;
  case GL_GREATER:
    return GPU_EARLYDEPTH_GREATER;
  case GL_GEQUAL:
    return GPU_EARLYDEPTH_GEQUAL;
  }

  Unreachable("Invalid early depth function!");
}

GPU_STENCILOP GLASS_utility_GLToGPUStencilOp(const GLenum op) {
  switch (op) {
  case GL_KEEP:
    return GPU_STENCIL_KEEP;
  case GL_ZERO:
    return GPU_STENCIL_ZERO;
  case GL_REPLACE:
    return GPU_STENCIL_REPLACE;
  case GL_INCR:
    return GPU_STENCIL_INCR;
  case GL_INCR_WRAP:
    return GPU_STENCIL_INCR_WRAP;
  case GL_DECR:
    return GPU_STENCIL_DECR;
  case GL_DECR_WRAP:
    return GPU_STENCIL_DECR_WRAP;
  case GL_INVERT:
    return GPU_STENCIL_INVERT;
  }

  Unreachable("Invalid stencil operation!");
}

GPU_BLENDEQUATION GLASS_utility_GLToGPUBlendEq(const GLenum eq) {
  switch (eq) {
  case GL_FUNC_ADD:
    return GPU_BLEND_ADD;
  case GL_MIN:
    return GPU_BLEND_MIN;
  case GL_MAX:
    return GPU_BLEND_MAX;
  case GL_FUNC_SUBTRACT:
    return GPU_BLEND_SUBTRACT;
  case GL_FUNC_REVERSE_SUBTRACT:
    return GPU_BLEND_REVERSE_SUBTRACT;
  }

  Unreachable("Invalid blend equation!");
}

GPU_BLENDFACTOR GLASS_utility_GLToGPUBlendFunc(const GLenum func) {
  switch (func) {
  case GL_ZERO:
    return GPU_ZERO;
  case GL_ONE:
    return GPU_ONE;
  case GL_SRC_COLOR:
    return GPU_SRC_COLOR;
  case GL_ONE_MINUS_SRC_COLOR:
    return GPU_ONE_MINUS_SRC_COLOR;
  case GL_DST_COLOR:
    return GPU_DST_COLOR;
  case GL_ONE_MINUS_DST_COLOR:
    return GPU_ONE_MINUS_DST_COLOR;
  case GL_SRC_ALPHA:
    return GPU_SRC_ALPHA;
  case GL_ONE_MINUS_SRC_ALPHA:
    return GPU_ONE_MINUS_SRC_ALPHA;
  case GL_DST_ALPHA:
    return GPU_DST_ALPHA;
  case GL_ONE_MINUS_DST_ALPHA:
    return GPU_ONE_MINUS_DST_ALPHA;
  case GL_CONSTANT_COLOR:
    return GPU_CONSTANT_COLOR;
  case GL_ONE_MINUS_CONSTANT_COLOR:
    return GPU_ONE_MINUS_CONSTANT_COLOR;
  case GL_CONSTANT_ALPHA:
    return GPU_CONSTANT_ALPHA;
  case GL_ONE_MINUS_CONSTANT_ALPHA:
    return GPU_ONE_MINUS_CONSTANT_ALPHA;
  case GL_SRC_ALPHA_SATURATE:
    return GPU_SRC_ALPHA_SATURATE;
  }

  Unreachable("Invalid blend function!");
}

GPU_LOGICOP GLASS_utility_GLToGPULOP(const GLenum op) {
  switch (op) {
  case GL_CLEAR:
    return GPU_LOGICOP_CLEAR;
  case GL_AND:
    return GPU_LOGICOP_AND;
  case GL_AND_REVERSE:
    return GPU_LOGICOP_AND_REVERSE;
  case GL_COPY:
    return GPU_LOGICOP_COPY;
  case GL_AND_INVERTED:
    return GPU_LOGICOP_AND_INVERTED;
  case GL_NOOP:
    return GPU_LOGICOP_NOOP;
  case GL_XOR:
    return GPU_LOGICOP_XOR;
  case GL_OR:
    return GPU_LOGICOP_OR;
  case GL_NOR:
    return GPU_LOGICOP_NOR;
  case GL_EQUIV:
    return GPU_LOGICOP_EQUIV;
  case GL_INVERT:
    return GPU_LOGICOP_INVERT;
  case GL_OR_REVERSE:
    return GPU_LOGICOP_OR_REVERSE;
  case GL_COPY_INVERTED:
    return GPU_LOGICOP_COPY_INVERTED;
  case GL_OR_INVERTED:
    return GPU_LOGICOP_OR_INVERTED;
  case GL_NAND:
    return GPU_LOGICOP_NAND;
  case GL_SET:
    return GPU_LOGICOP_SET;
  }

  Unreachable("Invalid operator!");
}

GPU_TEVSRC GLASS_utility_GLToGPUCombinerSrc(const GLenum src) {
  switch (src) {
  case GL_PRIMARY_COLOR:
    return GPU_PRIMARY_COLOR;
  case GL_FRAGMENT_PRIMARY_COLOR_PICA:
    return GPU_FRAGMENT_PRIMARY_COLOR;
  case GL_FRAGMENT_SECONDARY_COLOR_PICA:
    return GPU_FRAGMENT_SECONDARY_COLOR;
  case GL_TEXTURE0:
    return GPU_TEXTURE0;
  case GL_TEXTURE1:
    return GPU_TEXTURE1;
  case GL_TEXTURE2:
    return GPU_TEXTURE2;
  case GL_TEXTURE3:
    return GPU_TEXTURE3;
  case GL_PREVIOUS_BUFFER_PICA:
    return GPU_PREVIOUS_BUFFER;
  case GL_CONSTANT:
    return GPU_CONSTANT;
  case GL_PREVIOUS:
    return GPU_PREVIOUS;
  }

  Unreachable("Invalid combiner source!");
}

GPU_TEVOP_RGB GLASS_utility_GLToGPUCombinerOpRGB(const GLenum op) {
  switch (op) {
  case GL_SRC_COLOR:
    return GPU_TEVOP_RGB_SRC_COLOR;
  case GL_ONE_MINUS_SRC_COLOR:
    return GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR;
  case GL_SRC_ALPHA:
    return GPU_TEVOP_RGB_SRC_ALPHA;
  case GL_ONE_MINUS_SRC_ALPHA:
    return GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA;
  case GL_SRC_R_PICA:
    return GPU_TEVOP_RGB_SRC_R;
  case GL_ONE_MINUS_SRC_R_PICA:
    return GPU_TEVOP_RGB_ONE_MINUS_SRC_R;
  case GL_SRC_G_PICA:
    return GPU_TEVOP_RGB_SRC_G;
  case GL_ONE_MINUS_SRC_G_PICA:
    return GPU_TEVOP_RGB_ONE_MINUS_SRC_G;
  case GL_SRC_B_PICA:
    return GPU_TEVOP_RGB_SRC_B;
  case GL_ONE_MINUS_SRC_B_PICA:
    return GPU_TEVOP_RGB_ONE_MINUS_SRC_B;
  }

  Unreachable("Invalid combiner RGB operand!");
}

GPU_TEVOP_A GLASS_utility_GLToGPUCombinerOpAlpha(const GLenum op) {
  switch (op) {
  case GL_SRC_ALPHA:
    return GPU_TEVOP_A_SRC_ALPHA;
  case GL_ONE_MINUS_SRC_ALPHA:
    return GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA;
  case GL_SRC_R_PICA:
    return GPU_TEVOP_A_SRC_R;
  case GL_ONE_MINUS_SRC_R_PICA:
    return GPU_TEVOP_A_ONE_MINUS_SRC_R;
  case GL_SRC_G_PICA:
    return GPU_TEVOP_A_SRC_G;
  case GL_ONE_MINUS_SRC_G_PICA:
    return GPU_TEVOP_A_ONE_MINUS_SRC_G;
  case GL_SRC_B_PICA:
    return GPU_TEVOP_A_SRC_B;
  case GL_ONE_MINUS_SRC_B_PICA:
    return GPU_TEVOP_A_ONE_MINUS_SRC_B;
  }

  Unreachable("Invalid combiner alpha operand!");
}

GPU_COMBINEFUNC GLASS_utility_GLToGPUCombinerFunc(const GLenum func) {
  switch (func) {
  case GL_REPLACE:
    return GPU_REPLACE;
  case GL_MODULATE:
    return GPU_MODULATE;
  case GL_ADD:
    return GPU_ADD;
  case GL_ADD_SIGNED:
    return GPU_ADD_SIGNED;
  case GL_INTERPOLATE:
    return GPU_INTERPOLATE;
  case GL_SUBTRACT:
    return GPU_SUBTRACT;
  case GL_DOT3_RGB:
    return GPU_DOT3_RGB;
  case GL_DOT3_RGBA:
    return GPU_DOT3_RGB + 0x01;
  case GL_MULT_ADD_PICA:
    return GPU_MULTIPLY_ADD;
  case GL_ADD_MULT_PICA:
    return GPU_ADD_MULTIPLY;
  }

  Unreachable("Invalid combiner function!");
}

GPU_TEVSCALE GLASS_utility_GLToGPUCombinerScale(const GLfloat scale) {
  if (scale == 1.0f)
    return GPU_TEVSCALE_1;

  if (scale == 2.0f)
    return GPU_TEVSCALE_2;

  if (scale == 4.0f)
    return GPU_TEVSCALE_4;

  Unreachable("Invalid combiner scale!");
}

GPU_Primitive_t GLASS_utility_GLToGPUDrawMode(const GLenum mode) {
  switch (mode) {
  case GL_TRIANGLES:
    return GPU_TRIANGLES;
  case GL_TRIANGLE_STRIP:
    return GPU_TRIANGLE_STRIP;
  case GL_TRIANGLE_FAN:
    return GPU_TRIANGLE_FAN;
  case GL_GEOMETRY_PRIMITIVE_PICA:
    return GPU_GEOMETRY_PRIM;
  }

  Unreachable("Invalid draw mode!");
}

u32 GLASS_utility_GLToGPUDrawType(const GLenum type) {
  switch (type) {
  case GL_UNSIGNED_BYTE:
    return 0;
  case GL_UNSIGNED_SHORT:
    return 1;
  }

  Unreachable("Invalid draw type!");
}

u32 GLASS_utility_buildTransferFlags(const bool flipVertical, const bool tilted,
                                     const bool rawCopy,
                                     const GX_TRANSFER_FORMAT inputFormat,
                                     const GX_TRANSFER_FORMAT outputFormat,
                                     const GX_TRANSFER_SCALE scaling) {
  return GX_TRANSFER_FLIP_VERT(flipVertical) | GX_TRANSFER_OUT_TILED(tilted) |
         GX_TRANSFER_RAW_COPY(rawCopy) | GX_TRANSFER_IN_FORMAT(inputFormat) |
         GX_TRANSFER_OUT_FORMAT(outputFormat) | GX_TRANSFER_SCALING(scaling);
}

void GLASS_utility_clearBuffers(RenderbufferInfo *colorBuffer,
                                const u32 clearColor,
                                RenderbufferInfo *depthBuffer,
                                const u32 clearDepth) {
  size_t colorBufferSize = 0;
  size_t depthBufferSize = 0;

  if (colorBuffer) {
    colorBufferSize = colorBuffer->width * colorBuffer->height *
                      GetFBFormatBytes(colorBuffer->format);
  }

  if (depthBuffer) {
    depthBufferSize = depthBuffer->width * depthBuffer->height *
                      GetFBFormatBytes(depthBuffer->format);
  }

  if (colorBufferSize && depthBufferSize) {
    const bool colorFirst =
        (u32)colorBuffer->address < (u32)depthBuffer->address;
    if (colorFirst) {
      GX_MemoryFill((u32 *)colorBuffer->address, clearColor,
                    (u32 *)(colorBuffer->address + colorBufferSize),
                    GetGXControl(true, false, colorBuffer->format),
                    (u32 *)(depthBuffer->address), clearDepth,
                    (u32 *)(depthBuffer->address + depthBufferSize),
                    GetGXControl(true, false, depthBuffer->format));
    } else {
      GX_MemoryFill((u32 *)(depthBuffer->address), clearDepth,
                    (u32 *)(depthBuffer->address + depthBufferSize),
                    GetGXControl(true, false, depthBuffer->format),
                    (u32 *)(colorBuffer->address), clearColor,
                    (u32 *)(colorBuffer->address + colorBufferSize),
                    GetGXControl(true, false, colorBuffer->format));
    }
  } else if (colorBufferSize) {
    GX_MemoryFill((u32 *)(colorBuffer->address), clearColor,
                  (u32 *)(colorBuffer->address + colorBufferSize),
                  GetGXControl(true, false, colorBuffer->format), NULL, 0, NULL,
                  0);
  } else if (depthBufferSize) {
    GX_MemoryFill((u32 *)(depthBuffer->address), clearDepth,
                  (u32 *)(depthBuffer->address + depthBufferSize),
                  GetGXControl(true, false, depthBuffer->format), NULL, 0, NULL,
                  0);
  }
}

void GLASS_utility_transferBuffer(const RenderbufferInfo *colorBuffer,
                                  const RenderbufferInfo *displayBuffer,
                                  const u32 flags) {
  Assert(colorBuffer, "Color buffer was nullptr!");
  Assert(displayBuffer, "Display buffer was nullptr!");
  GX_DisplayTransfer((u32 *)(colorBuffer->address),
                     GX_BUFFER_DIM(colorBuffer->height, colorBuffer->width),
                     (u32 *)(displayBuffer->address),
                     GX_BUFFER_DIM(displayBuffer->height, displayBuffer->width),
                     flags);
}

void GLASS_utility_packIntVector(const u32 *in, u32 *out) {
  *out |= in[0] & 0xFF;
  *out |= (in[1] & 0xFF) << 8;
  *out |= (in[2] & 0xFF) << 16;
  *out |= (in[3] & 0xFF) << 24;
}

void GLASS_utility_packFloatVector(const float *in, u32 *out) {
  const u32 cvtX = f32tof24(in[0]);
  const u32 cvtY = f32tof24(in[1]);
  const u32 cvtZ = f32tof24(in[2]);
  const u32 cvtW = f32tof24(in[3]);
  out[0] = (cvtW << 8) | ((cvtZ >> 16) & 0xFF);
  out[1] = (cvtZ << 16) | ((cvtY >> 8) & 0xFFFF);
  out[2] = (cvtY << 16) | (cvtX & 0xFFFFFF);
}

void GLASS_utility_setBoolUniform(UniformInfo *info, const u16 mask,
                                  const size_t offset, const size_t size) {
  Assert(info, "Info was nullptr!");
  Assert(info->type == GLASS_UNI_BOOL, "Invalid uniform type!");
  Assert(info->count < 16, "Invalid bool uniform count!");
  Assert(offset < info->count, "Invalid offset!");
  Assert(size <= (info->count - offset), "Invalid size!");

  for (size_t i = offset; i < size; i++) {
    if ((mask >> (i - offset)) & 1)
      info->data.mask |= (1 << i);
    else
      info->data.mask &= ~(1 << i);
  }

  info->dirty = true;
}

void GLASS_utility_setIntUniform(UniformInfo *info, const u32 *vectorData,
                                 const size_t offset, const size_t size) {
  Assert(info, "Info was nullptr!");
  Assert(info->type == GLASS_UNI_INT, "Invalid uniform type!");
  Assert(info->count < 4, "Invalid int uniform count!");
  Assert(offset < info->count, "Invalid offset!");
  Assert(size <= (info->count - offset), "Invalid size!");

  if (info->count == 1) {
    info->data.value = vectorData[0];
  } else {
    for (size_t i = offset; i < size; i++)
      info->data.values[i] = vectorData[i - offset];
  }

  info->dirty = true;
}

void GLASS_utility_setFloatUniform(UniformInfo *info, const u32 *vectorData,
                                   const size_t offset, const size_t size) {
  Assert(info, "Info was nullptr!");
  Assert(info->type == GLASS_UNI_FLOAT, "Invalid uniform type!");
  Assert(info->count < 96, "Invalid float uniform count!");
  Assert(offset < info->count, "Invalid offset!");
  Assert(size <= (info->count - offset), "Invalid size!");

  for (size_t i = offset; i < size; i++) {
    CopyMem(&vectorData[4 * (i - offset)], &info->data.values[3 * i],
            3 * sizeof(u32));
  }

  info->dirty = true;
}