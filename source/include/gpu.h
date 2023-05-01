/**
 * gpu.h
 * Contains functions for managing the GPU.
 */
#ifndef _GLASS_GPU_H
#define _GLASS_GPU_H

#include "types.h"

// Initialize.
#define GPUInit GLASS_gpu_init
void GLASS_gpu_init(CtxImpl *ctx);

// Finalize.
#define GPUFini GLASS_gpu_fini
void GLASS_gpu_fini(CtxImpl *ctx);

// Enable writing to GPU registers.
#define GPUEnableRegs GLASS_gpu_enableRegs
void GLASS_gpu_enableRegs(CtxImpl *ctx);

// Disable writing to GPU registers.
#define GPUDisableRegs GLASS_gpu_disableRegs
void GLASS_gpu_disableRegs(CtxImpl *ctx);

// Flush pending GPU commands.
#define GPUFlushCommands GLASS_gpu_flushCommands
void GLASS_gpu_flushCommands(CtxImpl *ctx);

// Flush pending GPU commands and run them.
#define GPUFlushAndRunCommands GLASS_gpu_flushAndRunCommands
void GLASS_gpu_flushAndRunCommands(CtxImpl *ctx);

// Bind framebuffer.
#define BindFramebuffer GLASS_gpu_bindFramebuffer
void GLASS_gpu_bindFramebuffer(const FramebufferInfo *info, bool block32);

// Flush framebuffer.
#define FlushFramebuffer GLASS_gpu_flushFramebuffer
void GLASS_gpu_flushFramebuffer(void);

// Invalidate framebuffer.
#define InvalidateFramebuffer GLASS_gpu_invalidateFramebuffer
void GLASS_gpu_invalidateFramebuffer(void);

// Set viewport.
#define SetViewport GLASS_gpu_setViewport
void GLASS_gpu_setViewport(const GLint x, const GLint y, const GLsizei width,
                           const GLsizei height);

// Set scissor test.
#define SetScissorTest GLASS_gpu_setScissorTest
void GLASS_gpu_setScissorTest(const GPU_SCISSORMODE mode, const GLint x,
                              const GLint y, const GLsizei width,
                              const GLsizei height);

// Bind shaders.
#define BindShaders GLASS_gpu_bindShaders
void GLASS_gpu_bindShaders(const ShaderInfo *vertexShader,
                           const ShaderInfo *geometryShader);

// Upload all const uniforms.
#define UploadConstUniforms GLASS_gpu_uploadConstUniforms
void GLASS_gpu_uploadConstUniforms(const ShaderInfo *shader);

// Upload all uniforms.
#define UploadUniforms GLASS_gpu_uploadUniforms
void GLASS_gpu_uploadUniforms(ShaderInfo *shader);

// Upload all attributes.
#define UploadAttributes GLASS_gpu_uploadAttributes
void GLASS_gpu_uploadAttributes(const AttributeInfo *attribs,
                                const size_t *slots);

// Set texture combiners.
#define SetCombiners GLASS_gpu_setCombiners
void GLASS_gpu_setCombiners(const CombinerInfo *combiners);

// Set fragment operation.
#define SetFragOp GLASS_gpu_setFragOp
void GLASS_gpu_setFragOp(const GLenum fragMode, const bool blendMode);

// Set color and depth mask.
#define SetColorDepthMask GLASS_gpu_setColorDepthMask
void GLASS_gpu_setColorDepthMask(const bool writeRed, const bool writeGreen,
                                 const bool writeBlue, const bool writeAlpha,
                                 const bool writeDepth, const bool depthTest,
                                 const GLenum depthFunc);

// Set depth map.
#define SetDepthMap GLASS_gpu_setDepthMap
void GLASS_gpu_setDepthMap(const bool enabled, const GLclampf nearVal,
                           const GLclampf farVal, const GLfloat units,
                           const GLenum depthFormat);

// Enable early depth test.
#define SetEarlyDepthTest GLASS_gpu_setEarlyDepthTest
void GLASS_gpu_setEarlyDepthTest(const bool enabled);

// Set early depth test function.
#define SetEarlyDepthFunc GLASS_gpu_setEarlyDepthFunc
void GLASS_gpu_setEarlyDepthFunc(const GPU_EARLYDEPTHFUNC func);

// Set early depth clear value.
#define SetEarlyDepthClear GLASS_gpu_setEarlyDepthClear
void GLASS_gpu_setEarlyDepthClear(const GLclampf value);

// Clear early depth buffer.
#define ClearEarlyDepthBuffer GLASS_gpu_clearEarlyDepthBuffer
void GLASS_gpu_clearEarlyDepthBuffer(void);

// Set stencil test.
#define SetStencilTest GLASS_gpu_setStencilTest
void GLASS_gpu_setStencilTest(const bool enabled, const GLenum func,
                              const GLint ref, const GLuint mask,
                              const GLuint writeMask);

// Set stencil operations.
#define SetStencilOp GLASS_gpu_setStencilOp
void GLASS_gpu_setStencilOp(const GLenum sfail, const GLenum dpfail,
                            const GLenum dppass);

// Set cull face.
#define SetCullFace GLASS_gpu_setCullFace
void GLASS_gpu_setCullFace(const bool enabled, const GLenum cullFace,
                           const GLenum frontFace);

// Set alpha test.
#define SetAlphaTest GLASS_gpu_setAlphaTest
void GLASS_gpu_setAlphaTest(const bool enabled, const GLenum func,
                            const GLclampf ref);

// Set blend function.
#define SetBlendFunc GLASS_gpu_setBlendFunc
void GLASS_gpu_setBlendFunc(const GLenum rgbEq, const GLenum alphaEq,
                            const GLenum srcColor, const GLenum dstColor,
                            const GLenum srcAlpha, const GLenum dstAlpha);

// Set blend color.
#define SetBlendColor GLASS_gpu_setBlendColor
void GLASS_gpu_setBlendColor(const u32 color);

// Set logic op.
#define SetLogicOp GLASS_gpu_setLogicOp
void GLASS_gpu_setLogicOp(const GLenum logicOp);

// Draw arrays.
#define DrawArrays GLASS_gpu_drawArrays
void GLASS_gpu_drawArrays(const GLenum mode, const GLint first,
                          const GLsizei count);

// Draw elements.
#define DrawElements GLASS_gpu_drawElements
void GLASS_gpu_drawElements(const GLenum mode, const GLsizei count,
                            const GLenum type, const GLvoid *indices);

#endif /* _GLASS_GPU_H */