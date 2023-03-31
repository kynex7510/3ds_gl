/**
 * First we initialize a buffer that holds GPU commands. We use a pointer to it,
 * the offset relative to it, and the size which is essentially the offset of
 * the offset. We use these two values to be able to split the main buffer into
 * multiple chunks, so we can ensure flow integrity between GX and GPU commands.
 * For example: glClear() is implemented by using a GX call to fill the buffer
 * with a specific color; when calling glClear() we have to prepare the current
 * GPU command list for processing, then we do our framebuffer fill. All GX
 * commands are always queued, until glassSwapBuffers() is invoked, and unless
 * glFinish() is called.
 */
#include "gpu.h"
#include "utility.h"

#define GPU_MAX_ENTRIES 0x4000
#define GX_MAX_ENTRIES 32

// Helpers

#define UploadShaderBinary GLASS_gpu_uploadShaderBinary
static void GLASS_gpu_uploadShaderBinary(const ShaderInfo *shader) {
  if (shader->sharedData) {
    // Set write offset for code upload.
    GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY)
                        ? GPUREG_GSH_CODETRANSFER_CONFIG
                        : GPUREG_VSH_CODETRANSFER_CONFIG,
                    0);

    // Write code.
    // TODO: check code words limit.
    GPUCMD_AddWrites((shader->flags & SHADER_FLAG_GEOMETRY)
                         ? GPUREG_GSH_CODETRANSFER_DATA
                         : GPUREG_VSH_CODETRANSFER_DATA,
                     shader->sharedData->binaryCode,
                     shader->sharedData->numOfCodeWords < 512
                         ? shader->sharedData->numOfCodeWords
                         : 512);

    // Finalize code.
    GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY)
                        ? GPUREG_GSH_CODETRANSFER_END
                        : GPUREG_VSH_CODETRANSFER_END,
                    1);

    // Set write offset for op descs.
    GPUCMD_AddWrite((shader->flags & SHADER_FLAG_GEOMETRY)
                        ? GPUREG_GSH_OPDESCS_CONFIG
                        : GPUREG_VSH_OPDESCS_CONFIG,
                    0);

    // Write op descs.
    GPUCMD_AddWrites((shader->flags & SHADER_FLAG_GEOMETRY)
                         ? GPUREG_GSH_OPDESCS_DATA
                         : GPUREG_VSH_OPDESCS_DATA,
                     shader->sharedData->opDescs,
                     shader->sharedData->numOfOpDescs < 128
                         ? shader->sharedData->numOfOpDescs
                         : 128);
  }
}

#define UploadBoolUniformMask GLASS_gpu_uploadBoolUniformMask
static void GLASS_gpu_uploadBoolUniformMask(const ShaderInfo *shader,
                                            const u16 mask) {
  const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY)
                      ? GPUREG_GSH_BOOLUNIFORM
                      : GPUREG_VSH_BOOLUNIFORM;
  GPUCMD_AddWrite(reg, 0x7FFF0000 | mask);
}

#define UploadConstIntUniforms GLASS_gpu_uploadConstIntUniforms
static void GLASS_gpu_uploadConstIntUniforms(const ShaderInfo *shader) {
  const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY)
                      ? GPUREG_GSH_INTUNIFORM_I0
                      : GPUREG_VSH_INTUNIFORM_I0;

  for (size_t i = 0; i < 4; i++) {
    if (!((shader->constIntMask >> i) & 1))
      continue;

    GPUCMD_AddWrite(reg + i, shader->constIntData[i]);
  }
}

#define UploadIntUniform GLASS_gpu_uploadIntUniform
static void GLASS_gpu_uploadIntUniform(const ShaderInfo *shader,
                                       UniformInfo *info) {
  const u32 reg = (shader->flags & SHADER_FLAG_GEOMETRY)
                      ? GPUREG_GSH_INTUNIFORM_I0
                      : GPUREG_VSH_INTUNIFORM_I0;

  if (info->count == 1) {
    GPUCMD_AddWrite(reg + info->ID, info->data.value);
  } else {
    GPUCMD_AddIncrementalWrites(reg + info->ID, info->data.values, info->count);
  }
}

#define UploadConstFloatUniforms GLASS_gpu_uploadConstFloatUniforms
static void GLASS_gpu_uploadConstFloatUniforms(const ShaderInfo *shader) {
  const u32 idReg = (shader->flags & SHADER_FLAG_GEOMETRY)
                        ? GPUREG_GSH_FLOATUNIFORM_CONFIG
                        : GPUREG_VSH_FLOATUNIFORM_CONFIG;

  const u32 dataReg = (shader->flags & SHADER_FLAG_GEOMETRY)
                          ? GPUREG_GSH_FLOATUNIFORM_DATA
                          : GPUREG_VSH_FLOATUNIFORM_DATA;

  for (size_t i = 0; i < shader->numOfConstFloatUniforms; i++) {
    const ConstFloatInfo *uni = &shader->constFloatUniforms[i];
    GPUCMD_AddWrite(idReg, uni->ID);
    GPUCMD_AddIncrementalWrites(dataReg, uni->data, 3);
  }
}

#define UploadFloatUniform GLASS_gpu_uploadFloatUniform
static void GLASS_gpu_uploadFloatUniform(const ShaderInfo *shader,
                                         UniformInfo *info) {
  const u32 idReg = (shader->flags & SHADER_FLAG_GEOMETRY)
                        ? GPUREG_GSH_FLOATUNIFORM_CONFIG
                        : GPUREG_VSH_FLOATUNIFORM_CONFIG;

  const u32 dataReg = (shader->flags & SHADER_FLAG_GEOMETRY)
                          ? GPUREG_GSH_FLOATUNIFORM_DATA
                          : GPUREG_VSH_FLOATUNIFORM_DATA;

  // ID is automatically incremented after each write.
  GPUCMD_AddWrite(idReg, info->ID);

  for (size_t i = 0; i < info->count; i++)
    GPUCMD_AddIncrementalWrites(dataReg, &info->data.values[i * 3], 3);
}

// GPU

void GLASS_gpu_init(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  // Setup GPU command buffer.
  ctx->cmdBuffer = (u32 *)AllocLinear(GPU_MAX_ENTRIES * 4);
  Assert(ctx->cmdBuffer, "Could not allocate GPU command buffer!");

  // Setup GX command queue.
  ctx->gxQueue.maxEntries = GX_MAX_ENTRIES;
  ctx->gxQueue.entries =
      (gxCmdEntry_s *)AllocMem(ctx->gxQueue.maxEntries * sizeof(gxCmdEntry_s));
  Assert(ctx->gxQueue.entries, "Could not allocate GX command queue!");
}

void GLASS_gpu_fini(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  // Free GX command queue.
  FreeMem(ctx->gxQueue.entries);

  // Free GPU command buffer.
  if (ctx->cmdBuffer)
    FreeLinear(ctx->cmdBuffer);
}

void GLASS_gpu_enableRegs(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");
  GPUCMD_SetBuffer(ctx->cmdBuffer + ctx->cmdBufferOffset,
                   GPU_MAX_ENTRIES - ctx->cmdBufferOffset, ctx->cmdBufferSize);
}

void GLASS_gpu_disableRegs(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  // Save current buffer size.
  GPUCMD_GetBuffer(NULL, NULL, &ctx->cmdBufferSize);
  GPUCMD_SetBuffer(NULL, 0, 0);
}

void GLASS_gpu_flushCommands(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  if (ctx->cmdBufferSize) {
    // Split command buffer.
    GPUEnableRegs(ctx);
    GPUCMD_Split(NULL, &ctx->cmdBufferSize);
    GPUCMD_SetBuffer(NULL, 0, 0);

    // Process GPU commands.
    GX_ProcessCommandList(ctx->cmdBuffer + ctx->cmdBufferOffset,
                          ctx->cmdBufferSize * 4, GX_CMDLIST_FLUSH);

    // Set new offset.
    ctx->cmdBufferOffset += ctx->cmdBufferSize;
    ctx->cmdBufferSize = 0;
  }
}

void GLASS_gpu_flushAndRunCommands(CtxImpl *ctx) {
  Assert(ctx, "Context was nullptr!");

  // Flush pending commands.
  GPUFlushCommands(ctx);

  // Ensure execution.
  gxCmdQueueWait(&ctx->gxQueue, -1);
  gxCmdQueueStop(&ctx->gxQueue);
  gxCmdQueueClear(&ctx->gxQueue);

  // Reset offset.
  ctx->cmdBufferOffset = 0;

  // Run queue.
  gxCmdQueueRun(&ctx->gxQueue);
}

void GLASS_gpu_bindFramebuffer(const FramebufferInfo *info, bool block32) {
  u8 *colorBuffer = NULL;
  u8 *depthBuffer = NULL;
  u32 width = 0;
  u32 height = 0;
  GLenum colorFormat = 0;
  GLenum depthFormat = 0;
  u32 params[4];

  ZeroVar(params);

  if (info) {
    if (info->colorBuffer) {
      colorBuffer = info->colorBuffer->address;
      width = info->colorBuffer->width;
      height = info->colorBuffer->height;
      colorFormat = info->colorBuffer->format;
    }

    if (info->depthBuffer) {
      depthBuffer = info->depthBuffer->address;
      depthFormat = info->depthBuffer->format;

      if (!info->colorBuffer) {
        width = info->colorBuffer->width;
        height = info->colorBuffer->height;
      }
    }
  }

  InvalidateFramebuffer();

  // Set depth buffer, color buffer and dimensions.
  params[0] = osConvertVirtToPhys(depthBuffer) >> 3;
  params[1] = osConvertVirtToPhys(colorBuffer) >> 3;
  params[2] = 0x01000000 | (((width - 1) & 0xFFF) << 12) | (height & 0xFFF);
  GPUCMD_AddIncrementalWrites(GPUREG_DEPTHBUFFER_LOC, params, 3);
  GPUCMD_AddWrite(GPUREG_RENDERBUF_DIM, params[2]);

  // Set buffer parameters.
  if (colorBuffer) {
    GPUCMD_AddWrite(GPUREG_COLORBUFFER_FORMAT,
                    (GLToGPUFBFormat(colorFormat) << 16) |
                        GetFBPixelSize(colorFormat));
    params[0] = params[1] = 1;
  } else {
    params[0] = params[1] = 0;
  }

  if (depthBuffer) {
    GPUCMD_AddWrite(GPUREG_DEPTHBUFFER_FORMAT, GLToGPUFBFormat(depthFormat));
    params[2] = params[3] = 1;
  } else {
    params[2] = params[3] = 0;
  }

  if (info)
    GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_BLOCK32, block32 ? 1 : 0);

  GPUCMD_AddIncrementalWrites(GPUREG_COLORBUFFER_READ, params, 4);
}

void GLASS_gpu_flushFramebuffer(void) {
  GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1);
}

void GLASS_gpu_invalidateFramebuffer(void) {
  GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);
}

void GLASS_gpu_setViewport(const GLint x, const GLint y, const GLsizei width,
                           const GLsizei height) {
  u32 data[4];

  data[0] = f32tof24(height / 2.0f);
  data[1] = f32tof31(2.0f / height) << 1;
  data[2] = f32tof24(width / 2.0f);
  data[3] = f32tof31(2.0f / width) << 1;

  GPUCMD_AddIncrementalWrites(GPUREG_VIEWPORT_WIDTH, data, 4);
  GPUCMD_AddWrite(GPUREG_VIEWPORT_XY, (y << 16) | (x & 0xFFFF));
}

void GLASS_gpu_setScissorTest(const GPU_SCISSORMODE mode, const GLint x,
                              const GLint y, const GLsizei width,
                              const GLsizei height) {
  GPUCMD_AddMaskedWrite(GPUREG_SCISSORTEST_MODE, 0x01, mode);
  if (mode != GPU_SCISSOR_DISABLE) {
    GPUCMD_AddWrite(GPUREG_SCISSORTEST_POS, (y << 16) | (x & 0xFFFF));
    GPUCMD_AddWrite(GPUREG_SCISSORTEST_DIM,
                    ((height - y - 1) << 16) | ((width - x - 1) & 0xFFFF));
  }
}

void GLASS_gpu_bindShaders(const ShaderInfo *vertexShader,
                           const ShaderInfo *geometryShader) {
  // Initialize geometry engine.
  GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x03, geometryShader ? 2 : 0);
  GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 0x03, 0);
  GPUCMD_AddMaskedWrite(GPUREG_VSH_COM_MODE, 0x01, geometryShader ? 1 : 0);

  if (vertexShader) {
    // Upload binary for vertex shader.
    UploadShaderBinary(vertexShader);

    // Set vertex shader entrypoint.
    GPUCMD_AddWrite(GPUREG_VSH_ENTRYPOINT,
                    0x7FFF0000 | (vertexShader->codeEntrypoint & 0xFFFF));

    // Set vertex shader outmask.
    GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_MASK, 0x03, vertexShader->outMask);

    // Set vertex shader outmap number.
    GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_TOTAL1, 0x01,
                          vertexShader->outTotal - 1);
    GPUCMD_AddMaskedWrite(GPUREG_VSH_OUTMAP_TOTAL2, 0x01,
                          vertexShader->outTotal - 1);
  }

  if (geometryShader) {
    // Upload binary for geometry shader.
    UploadShaderBinary(geometryShader);

    // Set geometry shader entrypoint.
    GPUCMD_AddWrite(GPUREG_GSH_ENTRYPOINT,
                    0x7FFF0000 | (geometryShader->codeEntrypoint & 0xFFFF));

    // Set geometry shader outmask.
    GPUCMD_AddMaskedWrite(GPUREG_GSH_OUTMAP_MASK, 0x01,
                          geometryShader->outMask);
  }

  // Handle outmaps.
  u16 mergedOutTotal = 0;
  u32 mergedOutClock = 0;
  u32 mergedOutSems[7];
  bool useTexcoords = false;

  if (vertexShader && geometryShader &&
      (geometryShader->flags & SHADER_FLAG_MERGE_OUTMAPS)) {
    // Merge outmaps.
    for (size_t i = 0; i < 7; i++) {
      const u32 vshOutSem = vertexShader->outSems[i];
      u32 gshOutSem = geometryShader->outSems[i];

      if (vshOutSem != gshOutSem) {
        gshOutSem = gshOutSem != 0x1F1F1F1F ? gshOutSem : vshOutSem;
      }

      mergedOutSems[i] = gshOutSem;

      if (gshOutSem != 0x1F1F1F1F)
        ++mergedOutTotal;
    }

    mergedOutClock = vertexShader->outClock | geometryShader->outClock;
    useTexcoords = (vertexShader->flags & SHADER_FLAG_USE_TEXCOORDS) |
                   (geometryShader->flags & SHADER_FLAG_USE_TEXCOORDS);
  } else {
    const ShaderInfo *mainShader =
        geometryShader ? geometryShader : vertexShader;
    if (mainShader) {
      mergedOutTotal = mainShader->outTotal;
      CopyMem(mainShader->outSems, mergedOutSems, 7 * sizeof(u32));
      mergedOutClock = mainShader->outClock;
      useTexcoords = mainShader->flags & SHADER_FLAG_USE_TEXCOORDS;
    }
  }

  if (mergedOutTotal) {
    GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x01, mergedOutTotal - 1);
    GPUCMD_AddMaskedWrite(GPUREG_SH_OUTMAP_TOTAL, 0x01, mergedOutTotal);
    GPUCMD_AddIncrementalWrites(GPUREG_SH_OUTMAP_O0, mergedOutSems, 7);
    GPUCMD_AddMaskedWrite(GPUREG_SH_OUTATTR_MODE, 0x01, useTexcoords ? 1 : 0);
    GPUCMD_AddWrite(GPUREG_SH_OUTATTR_CLOCK, mergedOutClock);
  }

  // Configure geostage (TODO!!!!).
  if (false /* geometryShader != NULL*/) {
    /*
      // Set variable mode processing.
      GPUCMD_AddMaskedWrite(
          GPUREG_GEOSTAGE_CONFIG, 0x0A,
          geometryShader->gsMode == GSH_VARIABLE_PRIM ? 0x80000000 : 0);

      // Set variable mode parameters.
      GPUCMD_AddWrite(GPUREG_GSH_MISC1,
                      geometryShader->gshMode == GSH_VARIABLE_PRIM
                          ? (gshDvle->gshVariableVtxNum - 1)
                          : 0);

      // Set processing mode.
      u32 param = geometryShader->gsMode;
      GPUCMD_AddWrite(GPUREG_GSH_MISC0, param);

      // Set input.
      GPUCMD_AddWrite(GPUREG_GSH_INPUTBUFFER_CONFIG,
                      0x08000000 | (geometryShader->gshMode ? 0x0100 : 0) |
                          (stride - 1));

      // Set permutation.
      GPUCMD_AddIncrementalWrites(GPUREG_GSH_ATTRIBUTES_PERMUTATION_LOW,
                                  sp->geoShaderInputPermutation, 2);
                                  */
  } else {
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 0x0A, 0);
    GPUCMD_AddWrite(GPUREG_GSH_MISC0, 0);
    GPUCMD_AddWrite(GPUREG_GSH_MISC1, 0);
    GPUCMD_AddWrite(GPUREG_GSH_INPUTBUFFER_CONFIG, 0xA0000000);
  }
}

void GLASS_gpu_uploadConstUniforms(const ShaderInfo *shader) {
  Assert(shader, "Shader was nullptr!");

  UploadBoolUniformMask(shader, shader->constBoolMask);
  UploadConstIntUniforms(shader);
  UploadConstFloatUniforms(shader);
}

void GLASS_gpu_uploadUniforms(ShaderInfo *shader) {
  Assert(shader, "Shader was nullptr!");

  bool uploadBool = false;
  u16 boolMask = shader->constBoolMask;
  for (size_t i = 0; i < shader->numOfActiveUniforms; i++) {
    UniformInfo *uni = &shader->activeUniforms[i];

    if (!uni->dirty)
      continue;

    switch (uni->type) {
    case GLASS_UNI_BOOL:
      boolMask |= uni->data.mask;
      uploadBool = true;
      break;
    case GLASS_UNI_INT:
      UploadIntUniform(shader, uni);
      break;
    case GLASS_UNI_FLOAT:
      UploadFloatUniform(shader, uni);
      break;
    default:
      Unreachable("Invalid uniform type!");
    }

    uni->dirty = false;
  }

  if (uploadBool)
    UploadBoolUniformMask(shader, boolMask);
}

/*
void GLASS_gpu_uploadAttributes(const AttributeInfo *attribs,
                                const size_t *slots) {
  Assert(attribs, "Attribs was nullptr!");

  u32 format[2];
  u32 permutation[2];
  size_t attribCount = 0;

  format[0] = format[1] = permutation[0] = permutation[1] = 0;

  // Handle params.
  for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; i++) {
    const size_t index = slots[i];

    if (index >= GLASS_NUM_ATTRIB_REGS)
      continue;

    const AttributeInfo *attrib = &attribs[index];
    GPU_FORMATS attribType = GLToGPUAttribType(attrib->type);

    if (attrib->physAddr) {
      if (i < 8)
        format[0] |= GPU_ATTRIBFMT(i, attrib->count - 1, attribType);
      else
        format[1] |= GPU_ATTRIBFMT(i - 8, attrib->count - 1, attribType);

      format[1] &= ~(1 << (16 + i));

    } else {
      format[1] |= (1 << (16 + i));
    }

    if (i < 8)
      permutation[0] |= (index << (4 * i));
    else
      permutation[1] |= (index << (4 * i));
    ++attribCount;
  }

  format[1] |= (attribCount << 28);

  GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFERS_FORMAT_LOW, format, 2);
  GPUCMD_AddMaskedWrite(GPUREG_VSH_INPUTBUFFER_CONFIG, 0x0B,
                        0xA0000000 | (attribCount - 1));
  GPUCMD_AddWrite(GPUREG_VSH_NUM_ATTR, attribCount - 1);
  GPUCMD_AddIncrementalWrites(GPUREG_VSH_ATTRIBUTES_PERMUTATION_LOW,
                              permutation, 2);

  // Handle attributes.
  if (attribCount) {
    const u32 base = GetLinearBase();
    GPUCMD_AddWrite(GPUREG_ATTRIBBUFFERS_LOC, base >> 3);

    for (size_t i = 0; i < GLASS_NUM_ATTRIB_SLOTS; ++i) {
      const size_t index = slots[i];

      if (index >= GLASS_NUM_ATTRIB_REGS)
        continue;

      const AttributeInfo *attrib = &attribs[index];

      if (attrib->physAddr) {
        u32 params[3];
        params[0] = attrib->physAddr - base;
        params[1] = 0; // TODO
        params[2] = ((u8)attrib->stride << 16) | ((u8)attrib->count <<
28); GPUCMD_AddIncrementalWrites(GPUREG_ATTRIBBUFFER0_OFFSET + (i * 0x03),
                                    params, 3);
      } else {
        u32 packed[3];
        PackFixedAttrib(attrib->components, packed);
        GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_INDEX, i);
        GPUCMD_AddIncrementalWrites(GPUREG_FIXEDATTRIB_DATA0, packed, 3);
      }
    }
  }
}
*/

void GLASS_gpu_setCombiners(const CombinerInfo *combiners) {
  Assert(combiners, "Combiners was nullptr!");

  const size_t offsets[GLASS_NUM_COMBINER_STAGES] = {
      GPUREG_TEXENV0_SOURCE, GPUREG_TEXENV1_SOURCE, GPUREG_TEXENV2_SOURCE,
      GPUREG_TEXENV3_SOURCE, GPUREG_TEXENV4_SOURCE, GPUREG_TEXENV5_SOURCE,
  };

  for (size_t i = 0; i < GLASS_NUM_COMBINER_STAGES; i++) {
    u32 params[5];
    const CombinerInfo *combiner = &combiners[i];

    params[0] = GLToGPUCombinerSrc(combiner->rgbSrc[0]);
    params[0] |= (GLToGPUCombinerSrc(combiner->rgbSrc[1]) << 4);
    params[0] |= (GLToGPUCombinerSrc(combiner->rgbSrc[2]) << 8);
    params[0] |= (GLToGPUCombinerSrc(combiner->alphaSrc[0]) << 16);
    params[0] |= (GLToGPUCombinerSrc(combiner->alphaSrc[1]) << 20);
    params[0] |= (GLToGPUCombinerSrc(combiner->alphaSrc[2]) << 24);
    params[1] = GLToGPUCombinerOpRGB(combiner->rgbOp[0]);
    params[1] |= (GLToGPUCombinerOpRGB(combiner->rgbOp[1]) << 4);
    params[1] |= (GLToGPUCombinerOpRGB(combiner->rgbOp[2]) << 8);
    params[1] |= (GLToGPUCombinerOpAlpha(combiner->alphaOp[0]) << 12);
    params[1] |= (GLToGPUCombinerOpAlpha(combiner->alphaOp[1]) << 16);
    params[1] |= (GLToGPUCombinerOpAlpha(combiner->alphaOp[2]) << 20);
    params[2] = GLToGPUCombinerFunc(combiner->rgbFunc);
    params[2] |= (GLToGPUCombinerFunc(combiner->alphaFunc) << 16);
    params[3] = combiner->color;
    params[4] = GLToGPUCombinerScale(combiner->rgbScale);
    params[4] |= (GLToGPUCombinerScale(combiner->alphaScale) << 16);

    GPUCMD_AddIncrementalWrites(offsets[i], params, 5);
  }
}

void GLASS_gpu_setFragOp(const GLenum fragMode, const bool blendMode) {
  GPU_FRAGOPMODE gpuFragMode;

  switch (fragMode) {
  case GL_FRAGOP_MODE_DEFAULT_PICA:
    gpuFragMode = GPU_FRAGOPMODE_GL;
    break;
  case GL_FRAGOP_MODE_SHADOW_PICA:
    gpuFragMode = GPU_FRAGOPMODE_SHADOW;
    break;
  case GL_FRAGOP_MODE_GAS_PICA:
    gpuFragMode = GPU_FRAGOPMODE_GAS_ACC;
    break;
  default:
    Unreachable("Invalid fragment mode!");
  }

  GPUCMD_AddMaskedWrite(GPUREG_COLOR_OPERATION, 0x07,
                        0xE40000 | (blendMode ? 0x100 : 0x0) | gpuFragMode);
}

void GLASS_gpu_setColorDepthMask(const bool writeRed, const bool writeGreen,
                                 const bool writeBlue, const bool writeAlpha,
                                 const bool writeDepth, const bool depthTest,
                                 const GLenum depthFunc) {
  u32 value = ((writeRed ? 0x0100 : 0x00) | (writeGreen ? 0x0200 : 0x00) |
               (writeBlue ? 0x0400 : 0x00) | (writeAlpha ? 0x0800 : 0x00));

  if (depthTest) {
    value |=
        (GLToGPUTestFunc(depthFunc) << 4) | (writeDepth ? 0x1000 : 0x00) | 1;
  }

  GPUCMD_AddMaskedWrite(GPUREG_DEPTH_COLOR_MASK, 0x03, value);
}

void GLASS_gpu_setDepthMap(const bool enabled, const GLclampf nearVal,
                           const GLclampf farVal, const GLfloat units,
                           const GLenum depthFormat) {
  float offset = 0.0f;
  Assert(nearVal >= 0.0f && nearVal <= 1.0f, "Invalid near value!");
  Assert(farVal >= 0.0f && farVal <= 1.0f, "Invalid far value!");

  switch (depthFormat) {
  case GL_DEPTH_COMPONENT16:
    offset = (units / 65535.0f);
    break;
  case GL_DEPTH_COMPONENT24_OES:
  case GL_DEPTH24_STENCIL8_EXT:
    offset = (units / 16777215.0f);
    break;
  }

  GPUCMD_AddMaskedWrite(GPUREG_DEPTHMAP_ENABLE, 0x01, enabled ? 1 : 0);
  if (enabled) {
    GPUCMD_AddWrite(GPUREG_DEPTHMAP_SCALE, f32tof24(nearVal - farVal));
    GPUCMD_AddWrite(GPUREG_DEPTHMAP_OFFSET, f32tof24(nearVal + offset));
  }
}

void GLASS_gpu_setEarlyDepthTest(const bool enabled) {
  GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST1, 0x01, enabled ? 1 : 0);
  GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_TEST2, 0x01, enabled ? 1 : 0);
}

void GLASS_gpu_setEarlyDepthFunc(const GPU_EARLYDEPTHFUNC func) {
  GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_FUNC, 0x01, func);
}

void GLASS_gpu_setEarlyDepthClear(const GLclampf value) {
  Assert(value >= 0.0f && value <= 1.0f, "Invalid early depth value!");
  GPUCMD_AddMaskedWrite(GPUREG_EARLYDEPTH_DATA, 0x07, 0xFFFFFF * value);
}

void GLASS_gpu_clearEarlyDepthBuffer(void) {
  GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1);
}

void GLASS_gpu_setStencilTest(const bool enabled, const GLenum func,
                              const GLint ref, const GLuint mask,
                              const GLuint writeMask) {
  u32 value = enabled ? 1 : 0;
  if (enabled) {
    value |= (GLToGPUTestFunc(func) << 4);
    value |= ((u8)writeMask << 8);
    value |= ((int8_t)ref << 16);
    value |= ((u8)mask << 24);
  }

  GPUCMD_AddWrite(GPUREG_STENCIL_TEST, value);
}

void GLASS_gpu_setStencilOp(const GLenum sfail, const GLenum dpfail,
                            const GLenum dppass) {
  GPUCMD_AddMaskedWrite(GPUREG_STENCIL_OP, 0x03,
                        GLToGPUStencilOp(sfail) |
                            (GLToGPUStencilOp(dpfail) << 4) |
                            (GLToGPUStencilOp(dppass) << 8));
}

void GLASS_gpu_setCullFace(const bool enabled, const GLenum cullFace,
                           const GLenum frontFace) {
  // Essentially:
  // - set FRONT-CCW for FRONT-CCW/BACK-CW;
  // - set BACK-CCW in all other cases.
  const GPU_CULLMODE mode = ((cullFace == GL_FRONT) != (frontFace == GL_CCW))
                                ? GPU_CULL_BACK_CCW
                                : GPU_CULL_FRONT_CCW;
  GPUCMD_AddMaskedWrite(GPUREG_FACECULLING_CONFIG, 0x01,
                        enabled ? mode : GPU_CULL_NONE);
}

void GLASS_gpu_setAlphaTest(const bool enabled, const GLenum func,
                            const GLclampf ref) {
  Assert(ref >= 0.0f && ref <= 1.0f, "Invalid reference value!");
  u32 value = enabled ? 1 : 0;
  if (enabled) {
    value |= (GLToGPUTestFunc(func) << 4);
    value |= ((u8)(ref * 0xFF) << 8);
  }

  GPUCMD_AddMaskedWrite(GPUREG_FRAGOP_ALPHA_TEST, 0x03, value);
}

void GLASS_gpu_setBlendFunc(const GLenum rgbEq, const GLenum alphaEq,
                            const GLenum srcColor, const GLenum dstColor,
                            const GLenum srcAlpha, const GLenum dstAlpha) {
  GPU_BLENDEQUATION gpuRGBEq = GLToGPUBlendEq(rgbEq);
  GPU_BLENDEQUATION gpuAlphaEq = GLToGPUBlendEq(alphaEq);
  GPU_BLENDFACTOR gpuSrcColor = GLToGPUBlendFunc(srcColor);
  GPU_BLENDFACTOR gpuDstColor = GLToGPUBlendFunc(dstColor);
  GPU_BLENDFACTOR gpuSrcAlpha = GLToGPUBlendFunc(srcAlpha);
  GPU_BLENDFACTOR gpuDstAlpha = GLToGPUBlendFunc(dstAlpha);

  GPUCMD_AddWrite(GPUREG_BLEND_FUNC, (gpuDstAlpha << 28) | (gpuSrcAlpha << 24) |
                                         (gpuDstColor << 20) |
                                         (gpuSrcColor << 16) |
                                         (gpuAlphaEq << 8) | gpuRGBEq);
}

void GLASS_gpu_setBlendColor(const u32 color) {
  GPUCMD_AddWrite(GPUREG_BLEND_COLOR, color);
}

void GLASS_gpu_setLogicOp(const GLenum op) {
  GPUCMD_AddMaskedWrite(GPUREG_LOGIC_OP, 0x01, GLToGPULOP(op));
}

void GLASS_gpu_drawArrays(const GLenum mode, const GLint first,
                          const GLsizei count) {
  GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2, GLToGPUDrawMode(mode));
  GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
  GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG, 0x80000000);
  GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);
  GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, first);
  GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 1);
  GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
  GPUCMD_AddWrite(GPUREG_DRAWARRAYS, 1);
  GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);
  GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 1, 0);
  GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);
}

void GLASS_gpu_drawElements(const GLenum mode, const GLsizei count,
                            const GLenum type, const GLvoid *indices) {
  const GPU_Primitive_t primitive = GLToGPUDrawMode(mode);
  const u32 gpuType = GLToGPUDrawType(type);
  const u32 physAddr = osConvertVirtToPhys(indices);
  Assert(physAddr, "Invalid physical address!");

  GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 2,
                        primitive != GPU_TRIANGLES ? primitive
                                                   : GPU_GEOMETRY_PRIM);

  GPUCMD_AddWrite(GPUREG_RESTART_PRIMITIVE, 1);
  GPUCMD_AddWrite(GPUREG_INDEXBUFFER_CONFIG,
                  (physAddr - GetLinearBase()) | (gpuType << 31));

  GPUCMD_AddWrite(GPUREG_NUMVERTICES, count);
  GPUCMD_AddWrite(GPUREG_VERTEX_OFFSET, 0);

  if (primitive == GPU_TRIANGLES) {
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0x100);
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0x100);
  }

  GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 0);
  GPUCMD_AddWrite(GPUREG_DRAWELEMENTS, 1);
  GPUCMD_AddMaskedWrite(GPUREG_START_DRAW_FUNC0, 1, 1);

  if (primitive == GPU_TRIANGLES) {
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG, 2, 0);
    GPUCMD_AddMaskedWrite(GPUREG_GEOSTAGE_CONFIG2, 2, 0);
  }

  GPUCMD_AddWrite(GPUREG_VTX_FUNC, 1);
  GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x8, 0);
  GPUCMD_AddMaskedWrite(GPUREG_PRIMITIVE_CONFIG, 0x8, 0);
}