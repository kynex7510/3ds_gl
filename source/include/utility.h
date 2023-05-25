/**
 * utility.h
 * Contains random utilities used by the whole library.
 */
#ifndef _GLASS_UTILITY_H
#define _GLASS_UTILITY_H

#include "mem.h"

#define NORETURN __attribute__((noreturn))

#define Min(x, y) (((x) < (y)) ? (x) : (y))
//#define Max(x, y) (((x) > (y)) ? (x) : (y))

// Debugging utilities.

#ifndef NDEBUG
#define Assert(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond))                                                               \
      Unreachable((msg));                                                      \
  } while (false)

#define Log GLASS_utility_log
void GLASS_utility_log(const char *msg);
#define Unreachable GLASS_utility_unreachable
void GLASS_utility_unreachable(const char *msg) NORETURN;
#else
#define Assert(cond, msg)
#define Log(msg)
#define Unreachable(msg) GLASS_utility_unreachable()
void GLASS_utility_unreachable(void) NORETURN;
#endif // NDEBUG

// Clamp float value between 0.0 and 1.0.
#define ClampFloat(f) ((f) < 0.0f ? 0.0f : ((f) > 1.0f ? 1.0f : (f)))

// Convert physical address to virtual.
#define osConvertPhysToVirt GLASS_utility_convertPhysToVirt
void *GLASS_utility_convertPhysToVirt(const u32 addr);

// Convert float24 to float.
#define f24tof32 GLASS_utility_f24tof32
float GLASS_utility_f24tof32(const u32 f);

// Get raw clear color value.
#define MakeClearColor GLASS_utility_makeClearColor
u32 GLASS_utility_makeClearColor(const GLenum format, const u32 color);

// Get raw clear depth value.
#define MakeClearDepth GLASS_utility_makeClearDepth
u32 GLASS_utility_makeClearDepth(const GLenum format, const GLclampf factor,
                                 const u8 stencil);

// Get number of bytes per pixel for the specified format.
#define GetBytesPerPixel GLASS_utility_getBytesPerPixel
size_t GLASS_utility_getBytesPerPixel(const GLenum format);

// Get pixel size for framebuffer format (0 = 16, 1 = 24, 2 = 32 bits).
#define GetPixelSizeForFB GLASS_utility_getPixelSizeForFB
size_t GLASS_utility_getPixelSizeForFB(const GLenum format);

// Convert GSP to GL framebuffer format.
#define WrapFBFormat GLASS_utility_wrapFBFormat
GLenum GLASS_utility_wrapFBFormat(const GSPGPU_FramebufferFormat format);

// Convert GL to framebuffer transfer format.
#define GetTransferFormatForFB GLASS_utility_getTransferFormatForFB
GX_TRANSFER_FORMAT GLASS_utility_getTransferFormatForFB(const GLenum format);

// Convert GL to GPU renderbuffer format.
#define GetRBFormat GLASS_utility_GetRBFormat
GPU_COLORBUF GLASS_utility_getRBFormat(const GLenum format);

// Convert GL to GPU attribute type.
#define GetTypeForAttribType GLASS_utility_getTypeForAttribType
GPU_FORMATS GLASS_utility_getTypeForAttribType(const GLenum type);

// Convert GL to GPU test function.
#define GetTestFunc GLASS_utility_getTestFunc
GPU_TESTFUNC GLASS_utility_getTestFunc(const GLenum func);

// Convert GL to GPU early depth function.
#define GetEarlyDepthFunc GLASS_utility_getEarlyDepthFunc
GPU_EARLYDEPTHFUNC GLASS_utility_getEarlyDepthFunc(const GLenum func);

// Convert GL to GPU stencil operation.
#define GetStencilOp GLASS_utility_getStencilOp
GPU_STENCILOP GLASS_utility_getStencilOp(const GLenum op);

// Convert GL to GPU blend equation.
#define GetBlendEq GLASS_utility_getBlendEq
GPU_BLENDEQUATION GLASS_utility_getBlendEq(const GLenum eq);

// Convert GL to GPU blend function.
#define GetBlendFactor GLASS_utility_getBlendFactor
GPU_BLENDFACTOR GLASS_utility_getBlendFactor(const GLenum func);

// Convert GL to GPU logic op format.
#define GetLogicOp GLASS_utility_getLogicOp
GPU_LOGICOP GLASS_utility_getLogicOp(const GLenum op);

// Convert GL to GPU combiner source.
#define GetCombinerSrc GLASS_utility_getCombinerSrc
GPU_TEVSRC GLASS_utility_getCombinerSrc(const GLenum src);

// Convert GL to GPU combiner RGB operand.
#define GetCombinerOpRGB GLASS_utility_getCombinerOpRGB
GPU_TEVOP_RGB GLASS_utility_getCombinerOpRGB(const GLenum op);

// Convert GL to GPU combiner alpha operand.
#define GetCombinerOpAlpha GLASS_utility_getCombinerOpAlpha
GPU_TEVOP_A GLASS_utility_getCombinerOpAlpha(const GLenum op);

// Convert GL to GPU combiner.
#define GetCombinerFunc GLASS_utility_getCombinerFunc
GPU_COMBINEFUNC GLASS_utility_getCombinerFunc(const GLenum combiner);

// Convert GL to GPU combiner scale.
#define GetCombinerScale GLASS_utility_getCombinerScale
GPU_TEVSCALE GLASS_utility_getCombinerScale(const GLfloat scale);

// Convert GL to GPU draw mode.
#define GetDrawPrimitive GLASS_utility_getDrawPrimitive
GPU_Primitive_t GLASS_utility_getDrawPrimitive(const GLenum mode);

// Convert GL to GPU draw type.
#define GetDrawType GLASS_utility_getDrawType
u32 GLASS_utility_getDrawType(const GLenum type);

// Make flags for a display transfer.
#define MakeTransferFlags GLASS_utility_makeTransferFlags
u32 GLASS_utility_makeTransferFlags(const bool flipVertical, const bool tilted,
                                    const bool rawCopy,
                                    const GX_TRANSFER_FORMAT inputFormat,
                                    const GX_TRANSFER_FORMAT outputFormat,
                                    const GX_TRANSFER_SCALE scaling);

// Pack int values into int8 vector.
#define PackIntVector GLASS_utility_packIntVector
void GLASS_utility_packIntVector(const u32 *in, u32 *out);

// Pack float values into float24 vector.
#define PackFloatVector GLASS_utility_packFloatVector
void GLASS_utility_packFloatVector(const float *in, u32 *out);

// Unpack int8 vector into int values.
#define UnpackIntVector GLASS_utility_unpackIntVector
void GLASS_utility_unpackIntVector(const u32 in, u32 *out);

// Unpack float24 vector into float values.
#define UnpackFloatVector GLASS_utility_unpackFloatVector
void GLASS_utility_unpackFloatVector(const u32 *in, float *out);

// Get boolean uniform.
#define GetBoolUniform GLASS_utility_getBoolUniform
bool GLASS_utility_getBoolUniform(const UniformInfo *info, const size_t offset);

// Get int uniform.
#define GetIntUniform GLASS_utility_getIntUniform
void GLASS_utility_getIntUniform(const UniformInfo *info, const size_t offset,
                                 u32 *out);

// Get float uniform.
#define GetFloatUniform GLASS_utility_getFloatUniform
void GLASS_utility_getFloatUniform(const UniformInfo *info, const size_t offset,
                                   u32 *out);

// Set boolean uniform.
#define SetBoolUniform GLASS_utility_setBoolUniform
void GLASS_utility_setBoolUniform(UniformInfo *info, const size_t offset,
                                  const bool enabled);

// Set int uniform.
#define SetIntUniform GLASS_utility_setIntUniform
void GLASS_utility_setIntUniform(UniformInfo *info, const size_t offset,
                                 const u32 vector);

// Set float uniform.
#define SetFloatUniform GLASS_utility_setFloatUniform
void GLASS_utility_setFloatUniform(UniformInfo *info, const size_t offset,
                                   const u32 *vectorData);

#endif /* _GLASS_UTILITY_H */