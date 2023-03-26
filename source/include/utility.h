/**
 * utility.h
 * Contains random utilities used by the whole library.
 */
#ifndef _GLASS_UTILITY_H
#define _GLASS_UTILITY_H

#include "mem.h"

#define NORETURN __attribute__((noreturn))

#define Min(x, y) (((x) < (y)) ? (x) : (y))
#define Max(x, y) (((x) > (y)) ? (x) : (y))

#if defined(NDEBUG)
#include <assert.h>
#define Assert(cond, msg) assert((cond) && (msg))
#else
#define Assert(cond, msg)
#endif

// Unreachable implementation.
#define Unreachable(msg) GLASS_utility_unreachable((msg))
void GLASS_utility_unreachable(const char *msg) NORETURN;

#define osConvertPhysToVirt GLASS_utility_convertPhysToVirt
void *GLASS_utility_convertPhysToVirt(const u32 addr);

// Get linear heap physical base address.
#define GetLinearBase GLASS_utility_getLinearBase
u32 GLASS_utility_getLinearBase(void);

// Convert float24 to float.
#define f24tof32 GLASS_utility_f24tof32
float GLASS_utility_f24tof32(const u32 f);

// Convert RGBA8 color into other formats.
#define ConvertRGBA8 GLASS_utility_convertRGBA8
uint32_t GLASS_utility_convertRGBA8(const GLenum format, const uint32_t color);

// Get raw clear depth value.
#define GetClearDepth GLASS_utility_getClearDepth
uint32_t GLASS_utility_getClearDepth(const GLenum format, const GLclampf factor,
                                     const uint8_t stencil);

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
#define GLtoGPUAttribType GLASS_utility_GLToGPUAttribType
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
uint32_t GLASS_utility_buildTransferFlags(const bool flipVertical,
                                          const bool tilted, const bool rawCopy,
                                          const GX_TRANSFER_FORMAT inputFormat,
                                          const GX_TRANSFER_FORMAT outputFormat,
                                          const GX_TRANSFER_SCALE scaling);

// Clear color-depth buffers with the specified value.
#define ClearBuffers GLASS_utility_clearBuffers
void GLASS_utility_clearBuffers(RenderbufferInfo *colorBuffer,
                                const uint32_t clearColor,
                                RenderbufferInfo *depthBuffer,
                                const uint32_t clearDepth);

// Copy a color buffer to the specified display buffer.
#define TransferBuffer GLASS_utility_transferBuffer
void GLASS_utility_transferBuffer(const RenderbufferInfo *colorBuffer,
                                  const RenderbufferInfo *displayBuffer,
                                  const uint32_t flags);

// Set boolean (array) uniform.
#define SetUniformBool GLASS_utility_setUniformBool
void GLASS_utility_setUniformBool(UniformInfo *info, const uint16_t mask);

// Set int (array) uniform.
#define SetUniformInt GLASS_utility_setUniformInt
void GLASS_utility_setUniformInts(UniformInfo *info, const uint32_t *values);

// Set float (array) uniform.
#define SetUniformFloat GLASS_utility_setUniformFloat
void GLASS_utility_setUniformFloat(UniformInfo *info, const float *values);

// Reset dirty flag for all uniforms of a shader.
#define CleanUniforms GLASS_utility_cleanUniforms
void GLASS_utility_cleanUniforms(ShaderInfo *shader);

// Pack fixed attribute components.
#define PackFixedAttrib GLASS_utility_packFixedAttrib
void GLASS_utility_packFixedAttrib(const GLfloat *components, u32 *out);

#endif /* _GLASS_UTILITY_H */