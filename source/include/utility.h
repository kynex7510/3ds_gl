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

// Convert physical address to virtual.
#define osConvertPhysToVirt GLASS_utility_convertPhysToVirt
void *GLASS_utility_convertPhysToVirt(const u32 addr);

// Convert float24 to float.
#define f24tof32 GLASS_utility_f24tof32
float GLASS_utility_f24tof32(const u32 f);

// Convert RGBA8 color into other formats.
#define ConvertRGBA8 GLASS_utility_convertRGBA8
u32 GLASS_utility_convertRGBA8(const GLenum format, const u32 color);

// Get raw clear depth value.
#define GetClearDepth GLASS_utility_getClearDepth
u32 GLASS_utility_getClearDepth(const GLenum format, const GLclampf factor,
                                const u8 stencil);

// Clamp float value between 0.0 and 1.0.
#define GLClampFloat GLASS_utility_GLClampFloat
float GLASS_utility_GLClampFloat(const float f);

// Get number of bytes to hold each framebuffer format component.
#define GetFBFormatBytes GLASS_utility_getFBFormatBytes
size_t GLASS_utility_getFBFormatBytes(const GLenum format);

// Get pixel size for framebuffer format (0 = 16, 1 = 24, 2 = 32 bits).
#define GetFBPixelSize GLASS_utility_getFBPixelSize
size_t GLASS_utility_getFBPixelSize(const GLenum format);

// Convert GSP to GL framebuffer format.
#define GSPToGLFBFormat GLASS_utility_GSPToGLFBFormat
GLenum GLASS_utility_GSPToGLFBFormat(const GSPGPU_FramebufferFormat format);

// Convert GL to GX framebuffer format.
#define GLToGXFBFormat GLASS_utility_GLToGXFBFormat
GX_TRANSFER_FORMAT GLASS_utility_GLToGXFBFormat(const GLenum format);

// Convert GL to GPU framebuffer format.
#define GLToGPUFBFormat GLASS_utility_GLToGPUFBFormat
GPU_COLORBUF GLASS_utility_GLToGPUFBFormat(const GLenum format);

// Convert GL to GPU attribute type.
#define GLToGPUAttribType GLASS_utility_GLToGPUAttribType
GPU_FORMATS GLASS_utility_GLToGPUAttribType(const GLenum type);

// Convert GL to GPU test function.
#define GLToGPUTestFunc GLASS_utility_GLToGPUTestFunc
GPU_TESTFUNC GLASS_utility_GLToGPUTestFunc(const GLenum func);

// Convert GL to GPU early depth function.
#define GLToGPUEarlyDepthFunc GLASS_utility_GLToGPUEarlyDepthFunc
GPU_EARLYDEPTHFUNC GLASS_utility_GLToGPUEarlyDepthFunc(const GLenum func);

// Convert GL to GPU stencil operation.
#define GLToGPUStencilOp GLASS_utility_GLToGPUStencilOp
GPU_STENCILOP GLASS_utility_GLToGPUStencilOp(const GLenum op);

// Convert GL to GPU blend equation.
#define GLToGPUBlendEq GLASS_utility_GLToGPUBlendEq
GPU_BLENDEQUATION GLASS_utility_GLToGPUBlendEq(const GLenum eq);

// Convert GL to GPU blend function.
#define GLToGPUBlendFunc GLASS_utility_GLToGPUBlendFunc
GPU_BLENDFACTOR GLASS_utility_GLToGPUBlendFunc(const GLenum func);

// Convert GL to GPU logic op format.
#define GLToGPULOP GLASS_utility_GLToGPULOP
GPU_LOGICOP GLASS_utility_GLToGPULOP(const GLenum op);

// Convert GL to GPU combiner source.
#define GLToGPUCombinerSrc GLASS_utility_GLToGPUCombinerSrc
GPU_TEVSRC GLASS_utility_GLToGPUCombinerSrc(const GLenum src);

// Convert GL to GPU combiner RGB operand.
#define GLToGPUCombinerOpRGB GLASS_utility_GLToGPUCombinerOpRGB
GPU_TEVOP_RGB GLASS_utility_GLToGPUCombinerOpRGB(const GLenum op);

// Convert GL to GPU combiner alpha operand.
#define GLToGPUCombinerOpAlpha GLASS_utility_GLToGPUCombinerOpAlpha
GPU_TEVOP_A GLASS_utility_GLToGPUCombinerOpAlpha(const GLenum op);

// Convert GL to GPU combiner.
#define GLToGPUCombinerFunc GLASS_utility_GLToGPUCombinerFunc
GPU_COMBINEFUNC GLASS_utility_GLToGPUCombinerFunc(const GLenum combiner);

// Convert GL to GPU combiner scale.
#define GLToGPUCombinerScale GLASS_utility_GLToGPUCombinerScale
GPU_TEVSCALE GLASS_utility_GLToGPUCombinerScale(const GLfloat scale);

// Convert GL to GPU draw mode.
#define GLToGPUDrawMode GLASS_utility_GLToGPUDrawMode
GPU_Primitive_t GLASS_utility_GLToGPUDrawMode(const GLenum mode);

// Convert GL to GPU draw type.
#define GLToGPUDrawType GLASS_utility_GLToGPUDrawType
u32 GLASS_utility_GLToGPUDrawType(const GLenum type);

// Build flags for a display transfer.
#define BuildTransferFlags GLASS_utility_buildTransferFlags
u32 GLASS_utility_buildTransferFlags(const bool flipVertical, const bool tilted,
                                     const bool rawCopy,
                                     const GX_TRANSFER_FORMAT inputFormat,
                                     const GX_TRANSFER_FORMAT outputFormat,
                                     const GX_TRANSFER_SCALE scaling);

// Clear color-depth buffers with the specified value.
#define ClearBuffers GLASS_utility_clearBuffers
void GLASS_utility_clearBuffers(RenderbufferInfo *colorBuffer,
                                const u32 clearColor,
                                RenderbufferInfo *depthBuffer,
                                const u32 clearDepth);

// Copy a color buffer to the specified display buffer.
#define TransferBuffer GLASS_utility_transferBuffer
void GLASS_utility_transferBuffer(const RenderbufferInfo *colorBuffer,
                                  const RenderbufferInfo *displayBuffer,
                                  const u32 flags);

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