/**
 * Shaders have a refcount. It's set to 1 at creation.
 * It's incremented:
 * - each time it's attached to a program;
 * - when linking programs (to be linked).
 *
 * It's decremented:
 * - when explicitly deleted (only the first time);
 * - each time it's detached from a program;
 * - when linking programs (already linked).
 *
 * Shared shader data also have a refcount. That is
 * incremented at shader initialization, and decremented
 * when a shader linked to it is unlinked/deleted.
 *
 * This should be evenly distributed.
 */
#include "context.h"
#include "utility.h"

#define DVLB_MIN_SIZE 0x08
#define DVLB_MAGIC "\x44\x56\x4C\x42"

#define DVLP_MIN_SIZE 0x28
#define DVLP_MAGIC "\x44\x56\x4C\x50"

#define DVLE_MIN_SIZE 0x40
#define DVLE_MAGIC "\x44\x56\x4C\x45"

// Types

typedef struct {
  u32 numOfDVLEs; // Number of DVLE entries.
  u32 *DVLETable; // Pointer to DVLE entries.
} DVLB;

typedef struct {
  u16 type; //  Constant type.
  u16 ID;   // Constant ID.
  union {
    u32 boolUniform; // Bool uniform value.
    u32 intUniform;  // Int uniform value.
    struct {
      u32 x; // Float24 uniform X component.
      u32 y; // Float24 uniform Y component.
      u32 z; // Float24 uniform Z component.
      u32 w; // Float24 uniform W component.
    } floatUniform;
  } data;
} DVLEConstEntry;

typedef struct {
  bool isGeometry;                     // This DVLE is for a geometry shader.
  bool mergeOutmaps;                   // Merge shader outmaps (geometry only).
  u32 entrypoint;                      // Code entrypoint.
  DVLE_geoShaderMode gsMode;           // Geometry shader mode.
  DVLEConstEntry *constUniforms;       // Constant uniform table.
  u32 numOfConstUniforms;              // Size of constant uniform table.
  DVLE_outEntry_s *outRegs;            // Output register table.
  u32 numOfOutRegs;                    // Size of output register table.
  DVLE_uniformEntry_s *activeUniforms; // Uniform table.
  u32 numOfActiveUniforms;             // Size of uniform table.
  char *symbolTable;                   // Symbol table.
  u32 sizeOfSymbolTable;               // Size of symbol table.
} DVLEInfo;

// Helpers

#define FreeUniformData GLASS_shaders_freeUniformData
static void GLASS_shaders_freeUniformData(ShaderInfo *shader) {
  Assert(shader, "Shader was nullptr!");

  for (size_t i = 0; i < shader->numOfActiveUniforms; i++) {
    UniformInfo *uni = &shader->activeUniforms[i];
    if (uni->type == GLASS_UNI_FLOAT ||
        (uni->type == GLASS_UNI_INT && uni->count > 1))
      FreeMem(uni->data.values);
  }

  FreeMem(shader->constFloatUniforms);
  FreeMem(shader->activeUniforms);
}

#define DecSharedDataRefc GLASS_shaders_decSharedDataRefc
static void GLASS_shaders_decSharedDataRefc(SharedShaderData *sharedData) {
  Assert(sharedData, "Shared shader data was nullptr!");

  if (sharedData->refc)
    --sharedData->refc;

  if (!sharedData->refc)
    FreeMem(sharedData);
}

#define DecShaderRefc GLASS_shaders_decShaderRefc
static void GLASS_shaders_decShaderRefc(ShaderInfo *shader) {
  Assert(shader, "Shader was nullptr!");

  if (shader->refc)
    --shader->refc;

  if (!shader->refc) {
    Assert(shader->flags & SHADER_FLAG_DELETE,
           "Attempted to delete unflagged shader!");

    if (shader->sharedData)
      DecSharedDataRefc(shader->sharedData);

    FreeUniformData(shader);
    FreeMem(shader);
  }
}

#define DetachFromProgram GLASS_shaders_detachFromProgram
static void GLASS_shaders_detachFromProgram(ProgramInfo *pinfo,
                                            ShaderInfo *sinfo) {
  Assert(pinfo, "Pinfo was nullptr!");
  Assert(sinfo, "Sinfo was nullptr!");

  const GLuint sobj = (GLuint)sinfo;

  if (sinfo->flags & SHADER_FLAG_GEOMETRY) {
    if (pinfo->attachedGeometry != sobj) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    pinfo->attachedGeometry = GLASS_INVALID_OBJECT;
  } else {
    if (pinfo->attachedVertex != sobj) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    pinfo->attachedVertex = GLASS_INVALID_OBJECT;
  }

  DecShaderRefc(sinfo);
}

#define FreeProgram GLASS_shaders_freeProgram
static void GLASS_shaders_freeProgram(ProgramInfo *info) {
  Assert(info, "Info was nullptr!");
  Assert(info->flags & PROGRAM_FLAG_DELETE,
         "Attempted to delete unflagged program!");

  if (ObjectIsShader(info->attachedVertex))
    DecShaderRefc((ShaderInfo *)info->attachedVertex);

  if (ObjectIsShader(info->attachedGeometry))
    DecShaderRefc((ShaderInfo *)info->attachedGeometry);

  if (ObjectIsShader(info->linkedVertex))
    DecShaderRefc((ShaderInfo *)info->linkedVertex);

  if (ObjectIsShader(info->linkedGeometry))
    DecShaderRefc((ShaderInfo *)info->linkedGeometry);

  FreeMem(info);
}

#define NumActiveUniforms GLASS_shaders_numActiveUniforms
static size_t GLASS_shaders_numActiveUniforms(ProgramInfo *info) {
  Assert(info, "Info was nullptr!");

  size_t numOfActiveUniforms = 0;

  if (ObjectIsShader(info->linkedVertex)) {
    ShaderInfo *vshad = (ShaderInfo *)info->linkedVertex;
    numOfActiveUniforms = vshad->numOfActiveUniforms;
  }

  if (ObjectIsShader(info->linkedGeometry)) {
    ShaderInfo *gshad = (ShaderInfo *)info->linkedGeometry;
    numOfActiveUniforms += gshad->numOfActiveUniforms;
  }

  return numOfActiveUniforms;
}

#define LenActiveUniforms GLASS_shaders_lenActiveUniforms
static size_t GLASS_shaders_lenActiveUniforms(ProgramInfo *info) {
  Assert(info, "Info was nullptr!");

  size_t lenOfActiveUniforms = 0;

  if (ObjectIsShader(info->linkedVertex)) {
    // Look in vertex shader.
    ShaderInfo *vshad = (ShaderInfo *)info->linkedVertex;
    for (size_t i = 0; i < vshad->numOfActiveUniforms; i++) {
      const UniformInfo *uni = &vshad->activeUniforms[i];
      const size_t len = strlen(uni->symbol);
      if (len > lenOfActiveUniforms)
        lenOfActiveUniforms = len;
    }

    // Look in geometry shader.
    if (ObjectIsShader(info->linkedGeometry)) {
      ShaderInfo *gshad = (ShaderInfo *)info->linkedGeometry;
      for (size_t i = 0; i < gshad->numOfActiveUniforms; i++) {
        const UniformInfo *uni = &gshad->activeUniforms[i];
        const size_t len = strlen(uni->symbol);
        if (len > lenOfActiveUniforms)
          lenOfActiveUniforms = len;
      }
    }
  }

  return lenOfActiveUniforms;
}

#define LookupShader GLASS_shaders_lookupShader
static size_t GLASS_shaders_lookupShader(const GLuint *shaders,
                                         const size_t maxShaders,
                                         const size_t index,
                                         const bool isGeometry) {
  Assert(shaders, "Shader array was nullptr!");

  for (size_t i = (index + 1); i < maxShaders; i++) {
    GLuint name = shaders[i];

    // Force failure if one of the shaders is invalid.
    if (!ObjectIsShader(name)) {
      return index;
    }

    ShaderInfo *shader = (ShaderInfo *)name;
    if (((shader->flags & SHADER_FLAG_GEOMETRY) == SHADER_FLAG_GEOMETRY) ==
        isGeometry)
      return i;
  }

  return index;
}

#define ParseDVLB GLASS_shaders_parseDVLB
static DVLB *GLASS_shaders_parseDVLB(const u8 *data, const size_t size) {
  Assert(data, "Data was nullptr!");
  Assert(size > DVLB_MIN_SIZE, "Invalid DVLB size!");
  Assert(EqMem(data, DVLB_MAGIC, 4), "Invalid DVLB header!");

  // Read number of DVLEs.
  u32 numOfDVLEs = 0;
  CopyMem(data + 0x04, &numOfDVLEs, 4);
  Assert((DVLB_MIN_SIZE + (numOfDVLEs * 4)) <= size, "DVLE table OOB!");

  // Allocate DVLB.
  const size_t sizeOfDVLB = sizeof(DVLB) + (numOfDVLEs * 4);
  DVLB *dvlb = (DVLB *)AllocMem(sizeOfDVLB);

  if (dvlb) {
    dvlb->numOfDVLEs = numOfDVLEs;
    dvlb->DVLETable = (u32 *)((u8 *)dvlb + sizeof(DVLB));

    // Fill table with offsets.
    for (size_t i = 0; i < numOfDVLEs; i++) {
      dvlb->DVLETable[i] = *(u32 *)(data + DVLB_MIN_SIZE + (4 * i));
      Assert(dvlb->DVLETable[i] <= size, "DVLE offset OOB!");
      // Relocation.
      dvlb->DVLETable[i] += (u32)data;
    }
  }

  return dvlb;
}

#define ParseDVLP GLASS_shaders_parseDVLP
static SharedShaderData *GLASS_shaders_parseDVLP(const u8 *data,
                                                 const size_t size) {
  Assert(data, "Data was nullptr!");
  Assert(size > DVLP_MIN_SIZE, "Invalid DVLP size!");
  Assert(EqMem(data, DVLP_MAGIC, 4), "Invalid DVLP header!");

  // Read offsets.
  u32 offsetToBlob = 0;
  u32 offsetToOpDescs = 0;
  CopyMem(data + 0x08, &offsetToBlob, 4);
  CopyMem(data + 0x10, &offsetToOpDescs, 4);
  Assert(offsetToBlob < size, "DVLP blob start offset OOB!");
  Assert(offsetToOpDescs < size, "DVLP opdescs start offset OOB!");

  // Read num params.
  u32 numOfCodeWords = 0;
  u32 numOfOpDescs = 0;
  CopyMem(data + 0x0C, &numOfCodeWords, 4);
  CopyMem(data + 0x14, &numOfOpDescs, 4);

  // TODO: check code words limit.
  Assert(numOfCodeWords <= 512, "Invalid num of DVLP code words!");
  Assert(numOfOpDescs <= 128, "Invalid num of DVLP opdescs!");
  Assert((offsetToBlob + (numOfCodeWords * 4)) <= size,
         "DVLP blob end offset OOB!");
  Assert((offsetToOpDescs + (numOfOpDescs * 8)) <= size,
         "DVLP opdescs end offset OOB!");

  // Allocate data.
  const size_t sizeOfData =
      sizeof(SharedShaderData) + (numOfCodeWords * 4) + (numOfOpDescs * 4);
  SharedShaderData *sharedData = (SharedShaderData *)AllocMem(sizeOfData);

  if (sharedData) {
    sharedData->refc = 0;
    sharedData->binaryCode =
        (u32 *)((u8 *)sharedData + sizeof(SharedShaderData));
    sharedData->numOfCodeWords = numOfCodeWords;
    sharedData->opDescs = sharedData->binaryCode + sharedData->numOfCodeWords;
    sharedData->numOfOpDescs = numOfOpDescs;

    // Relocation.
    offsetToBlob += (u32)data;
    offsetToOpDescs += (u32)data;

    // Read binary code.
    CopyMem(offsetToBlob, sharedData->binaryCode,
            sharedData->numOfCodeWords * 4);

    // Read op descs.
    for (size_t i = 0; i < sharedData->numOfOpDescs; i++)
      sharedData->opDescs[i] = ((u32 *)offsetToOpDescs)[i * 2];
  }

  return sharedData;
}

#define GetDVLEInfo GLASS_shaders_getDVLEInfo
static void GLASS_shaders_getDVLEInfo(const u8 *data, const size_t size,
                                      DVLEInfo *out) {
  Assert(data, "Data was nullptr!");
  Assert(out, "Out was nullptr!");
  Assert(size > DVLE_MIN_SIZE, "Invalid DVLE size!");
  Assert(EqMem(data, DVLE_MAGIC, 4), "Invalid DVLE header!");

  // Get info.
  u8 flags = 0;
  u8 mergeOutmaps = 0;
  u8 gsMode = 0;
  u32 offsetToConstTable = 0;
  u32 offsetToOutTable = 0;
  u32 offsetToUniformTable = 0;
  u32 offsetToSymbolTable = 0;
  CopyMem(data + 0x06, &flags, 1);
  CopyMem(data + 0x07, &mergeOutmaps, 1);
  CopyMem(data + 0x08, &out->entrypoint, 4);
  CopyMem(data + 0x14, &gsMode, 1);
  CopyMem(data + 0x18, &offsetToConstTable, 4);
  CopyMem(data + 0x1C, &out->numOfConstUniforms, 4);
  CopyMem(data + 0x28, &offsetToOutTable, 4);
  CopyMem(data + 0x2C, &out->numOfOutRegs, 4);
  CopyMem(data + 0x30, &offsetToUniformTable, 4);
  CopyMem(data + 0x34, &out->numOfActiveUniforms, 4);
  CopyMem(data + 0x38, &offsetToSymbolTable, 4);
  CopyMem(data + 0x3C, &out->sizeOfSymbolTable, 4);
  Assert(offsetToConstTable < size, "DVLE const table start offset OOB!");
  Assert(offsetToOutTable < size, "DVLE output table start offset OOB!");
  Assert(offsetToUniformTable < size, "DVLE uniform table start offset OOB!");
  Assert(offsetToSymbolTable < size, "DVLE symbol table start offset OOB!");
  Assert((offsetToConstTable + (out->numOfConstUniforms * 20)) <= size,
         "DVLE const table end offset OOB!");
  Assert((offsetToOutTable + (out->numOfOutRegs * 8)) <= size,
         "DVLE output table end offset OOB!");
  Assert((offsetToUniformTable + (out->numOfActiveUniforms * 8)) <= size,
         "DVLE uniform table end offset OOB!");
  Assert((offsetToSymbolTable + out->sizeOfSymbolTable) <= size,
         "DVLE symbol table end offset OOB!");

  // Handle geometry shader.
  switch (flags) {
  case 0x00:
    out->isGeometry = false;
    break;
  case 0x01:
    out->isGeometry = true;
    break;
  default:
    Unreachable("Unknown DVLE flags value!");
  }

  // Handle merge outmaps.
  if (mergeOutmaps & 1) {
    Assert(out->isGeometry, "Merge outmaps is geometry shader only!");
    out->mergeOutmaps = true;
  } else {
    out->mergeOutmaps = false;
  }

  // Handle geometry mode.
  if (out->isGeometry) {
    switch (gsMode) {
    case 0x00:
      out->gsMode = GSH_POINT;
      break;
    case 0x01:
      out->gsMode = GSH_VARIABLE_PRIM;
      break;
    case 0x02:
      out->gsMode = GSH_FIXED_PRIM;
      break;
    default:
      Unreachable("Unknown DVLE geometry shader mode!");
    }
  }

  // Relocation.
  offsetToConstTable += (u32)data;
  offsetToOutTable += (u32)data;
  offsetToUniformTable += (u32)data;
  offsetToSymbolTable += (u32)data;

  // Set table pointers.
  out->constUniforms = (DVLEConstEntry *)offsetToConstTable;
  out->outRegs = (DVLE_outEntry_s *)offsetToOutTable;
  out->activeUniforms = (DVLE_uniformEntry_s *)offsetToUniformTable;
  out->symbolTable = (char *)offsetToSymbolTable;
}

#define GenerateOutmaps GLASS_shaders_generateOutmaps
static void GLASS_shaders_generateOutmaps(const DVLEInfo *info,
                                          ShaderInfo *out) {
  Assert(info, "Info was nullptr!");
  Assert(out, "Out was nullptr!");

  bool useTexcoords = false;
  out->outMask = 0;
  out->outTotal = 0;
  out->outClock = 0;
  SetMem(out->outSems, sizeof(out->outSems), 0x1F);

  for (size_t i = 0; i < info->numOfOutRegs; i++) {
    const DVLE_outEntry_s *entry = &info->outRegs[i];
    u8 sem = 0x1F;
    size_t maxSem = 0;

    // Set output register.
    if (!(out->outMask & (1 << entry->regID))) {
      out->outMask |= (1 << entry->regID);
      ++out->outTotal;
    }

    // Get register semantics.
    switch (entry->type) {
    case RESULT_POSITION:
      sem = 0x00;
      maxSem = 4;
      break;
    case RESULT_NORMALQUAT:
      sem = 0x04;
      maxSem = 4;
      out->outClock |= (1 << 24);
      break;
    case RESULT_COLOR:
      sem = 0x08;
      maxSem = 4;
      out->outClock |= (1 << 1);
      break;
    case RESULT_TEXCOORD0:
      sem = 0x0C;
      maxSem = 2;
      out->outClock |= (1 << 8);
      useTexcoords = true;
      break;
    case RESULT_TEXCOORD0W:
      sem = 0x10;
      maxSem = 1;
      out->outClock |= (1 << 16);
      useTexcoords = true;
      break;
    case RESULT_TEXCOORD1:
      sem = 0x0E;
      maxSem = 2;
      out->outClock |= (1 << 9);
      useTexcoords = true;
      break;
    case RESULT_TEXCOORD2:
      sem = 0x16;
      maxSem = 2;
      out->outClock |= (1 << 10);
      useTexcoords = true;
      break;
    case RESULT_VIEW:
      sem = 0x12;
      maxSem = 3;
      out->outClock |= (1 << 24);
      break;
    case RESULT_DUMMY:
      continue;
    default:
      Unreachable("Unknown output register type!");
    }

    // Set register semantics.
    for (size_t i = 0, curSem = 0; i < 4 && curSem < maxSem; i++) {
      if (entry->mask & (1 << i)) {
        out->outSems[entry->regID] &= ~(0xFF << (i * 8));
        out->outSems[entry->regID] |= ((sem++) << (i * 8));

        // Check for position.z clock.
        ++curSem;
        if (entry->type == RESULT_POSITION && curSem == 3)
          out->outClock |= (1 << 0);
      }
    }
  }

  if (useTexcoords)
    out->flags |= SHADER_FLAG_USE_TEXCOORDS;
  else
    out->flags &= ~SHADER_FLAG_USE_TEXCOORDS;
}

#define LoadUniforms GLASS_shaders_loadUniforms
bool GLASS_shaders_loadUniforms(const DVLEInfo *info, ShaderInfo *out) {
  Assert(info, "Info was nullptr!");
  Assert(out, "Out was nullptr!");

  // Cleanup.
  FreeUniformData(out);
  out->constBoolMask = 0;
  ZeroVar(out->constIntData);
  out->constIntMask = 0;
  out->constFloatUniforms = NULL;
  out->numOfConstFloatUniforms = 0;
  out->activeUniforms = NULL;
  out->numOfActiveUniforms = 0;

  // Setup constant uniforms.
  size_t numOfConstFloatUniforms = 0;
  for (size_t i = 0; i < info->numOfConstUniforms; i++) {
    const DVLEConstEntry *constEntry = &info->constUniforms[i];

    switch (constEntry->type) {
    case GLASS_UNI_BOOL:
      Assert(constEntry->ID < GLASS_NUM_BOOL_UNIFORMS,
             "Invalid const bool uniform ID!");
      if (constEntry->data.boolUniform)
        out->constBoolMask |= (1 << constEntry->ID);
      break;
    case GLASS_UNI_INT:
      Assert(constEntry->ID < GLASS_NUM_INT_UNIFORMS,
             "Invalid const int uniform ID!");
      out->constIntData[constEntry->ID] = constEntry->data.intUniform;
      out->constIntMask |= (1 << constEntry->ID);
      break;
    case GLASS_UNI_FLOAT:
      Assert(constEntry->ID < GLASS_NUM_FLOAT_UNIFORMS,
             "Invalid const float uniform ID!");
      ++numOfConstFloatUniforms;
      break;
    default:
      Unreachable("Unknown const uniform type!");
    }
  }

  out->constFloatUniforms = (ConstFloatInfo *)AllocMem(sizeof(ConstFloatInfo) *
                                                       numOfConstFloatUniforms);
  if (!out->constFloatUniforms)
    return false;

  out->numOfConstFloatUniforms = numOfConstFloatUniforms;

  for (size_t i = 0; i < out->numOfConstFloatUniforms; i++) {
    const DVLEConstEntry *constEntry = &info->constUniforms[i];
    if (constEntry->type != GLASS_UNI_FLOAT)
      continue;

    const float components[4] = {f24tof32(constEntry->data.floatUniform.x),
                                 f24tof32(constEntry->data.floatUniform.y),
                                 f24tof32(constEntry->data.floatUniform.z),
                                 f24tof32(constEntry->data.floatUniform.w)};

    ConstFloatInfo *uni = &out->constFloatUniforms[i];
    uni->ID = constEntry->ID;
    PackFloatVector(components, uni->data);
  }

  // Setup active uniforms.
  out->activeUniforms =
      (UniformInfo *)AllocMem(sizeof(UniformInfo) * info->numOfActiveUniforms);
  if (!out->activeUniforms)
    return false;

  out->numOfActiveUniforms = info->numOfActiveUniforms;

  for (size_t i = 0; i < info->numOfActiveUniforms; i++) {
    UniformInfo *uni = &out->activeUniforms[i];
    const DVLE_uniformEntry_s *entry = &info->activeUniforms[i];

    uni->ID = entry->startReg;
    uni->count = (entry->endReg + 1) - entry->startReg;
    uni->symbol = out->symbolTable + entry->symbolOffset;

    // Handle bool.
    if (entry->startReg >= 0x78 && entry->startReg <= 0x87) {
      Assert(entry->endReg <= 0x87, "Invalid bool uniform range!");
      uni->type = GLASS_UNI_BOOL;
      uni->data.mask = 0;
      continue;
    }

    // Handle int.
    if (entry->startReg >= 0x70 && entry->startReg <= 0x73) {
      Assert(entry->endReg <= 0x73, "Invalid int uniform range!");
      uni->type = GLASS_UNI_INT;

      if (uni->count > 1) {
        uni->data.values = (u32 *)AllocMem(sizeof(u32) * uni->count);
        if (!uni->data.values)
          return false;
      } else {
        uni->data.value = 0;
      }

      continue;
    }

    // Handle float.
    if (entry->startReg >= 0x10 && entry->startReg <= 0x6F) {
      Assert(entry->endReg <= 0x6F, "Invalid float uniform range!");
      uni->type = GLASS_UNI_FLOAT;
      uni->data.values = (u32 *)AllocMem((sizeof(u32) * 3) * uni->count);
      if (!uni->data.values)
        return false;

      continue;
    }

    Unreachable("Unknown uniform type!");
  }

  return true;
}

// Shaders

void glAttachShader(GLuint program, GLuint shader) {
  if (!ObjectIsProgram(program) || !ObjectIsShader(shader)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *pinfo = (ProgramInfo *)program;
  ShaderInfo *sinfo = (ShaderInfo *)shader;

  // Attach shader to program.
  if (sinfo->flags & SHADER_FLAG_GEOMETRY) {
    if (ObjectIsShader(pinfo->attachedGeometry)) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    pinfo->attachedGeometry = shader;
  } else {
    if (ObjectIsShader(pinfo->attachedVertex)) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    pinfo->attachedVertex = shader;
  }

  // Increment shader refcount.
  ++sinfo->refc;
}

GLuint glCreateProgram(void) {
  GLuint name = CreateObject(GLASS_PROGRAM_TYPE);
  if (ObjectIsProgram(name)) {
    ProgramInfo *info = (ProgramInfo *)name;
    info->attachedVertex = GLASS_INVALID_OBJECT;
    info->linkedVertex = GLASS_INVALID_OBJECT;
    info->attachedGeometry = GLASS_INVALID_OBJECT;
    info->linkedGeometry = GLASS_INVALID_OBJECT;
    info->flags = 0;
    return name;
  }

  SetError(GL_OUT_OF_MEMORY);
  return GLASS_INVALID_OBJECT;
}

GLuint glCreateShader(GLenum shaderType) {
  u16 flags = 0;

  switch (shaderType) {
  case GL_VERTEX_SHADER:
    break;
  case GL_GEOMETRY_SHADER_PICA:
    flags = SHADER_FLAG_GEOMETRY;
    break;
  default:
    SetError(GL_INVALID_ENUM);
    return GLASS_INVALID_OBJECT;
  }

  // Create shader.
  GLuint name = CreateObject(GLASS_SHADER_TYPE);
  if (ObjectIsShader(name)) {
    ShaderInfo *info = (ShaderInfo *)name;
    info->flags = flags;
    info->refc = 1;
    return name;
  }

  SetError(GL_OUT_OF_MEMORY);
  return GLASS_INVALID_OBJECT;
}

void glDeleteProgram(GLuint program) {
  // A value of 0 is silently ignored.
  if (program == GLASS_INVALID_OBJECT)
    return;

  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  CtxImpl *ctx = GetContext();
  ProgramInfo *info = (ProgramInfo *)program;

  // Flag for deletion.
  if (!(info->flags & PROGRAM_FLAG_DELETE)) {
    info->flags |= PROGRAM_FLAG_DELETE;
    if (ctx->currentProgram != program)
      FreeProgram(info);
  }
}

void glDeleteShader(GLuint shader) {
  // A value of 0 is silently ignored.
  if (shader == GLASS_INVALID_OBJECT)
    return;

  if (!ObjectIsShader(shader)) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  ShaderInfo *info = (ShaderInfo *)shader;

  // Flag for deletion.
  if (!(info->flags & SHADER_FLAG_DELETE)) {
    info->flags |= SHADER_FLAG_DELETE;
    DecShaderRefc(info);
  }
}

void glDetachShader(GLuint program, GLuint shader) {
  if (!ObjectIsProgram(program) || !ObjectIsShader(shader)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *pinfo = (ProgramInfo *)program;
  ShaderInfo *sinfo = (ShaderInfo *)shader;
  DetachFromProgram(pinfo, sinfo);
}

void glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count,
                          GLuint *shaders) {
  Assert(shaders, "Shaders was nullptr!");

  GLsizei shaderCount = 0;

  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  if (maxCount < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  ProgramInfo *info = (ProgramInfo *)program;

  // Get shaders.
  if (shaderCount < maxCount) {
    if (ObjectIsShader(info->attachedVertex))
      shaders[shaderCount++] = info->attachedVertex;
  }

  if (shaderCount < maxCount) {
    if (ObjectIsShader(info->attachedGeometry))
      shaders[shaderCount++] = info->attachedGeometry;
  }

  // Get count.
  if (count)
    *count = shaderCount;
}

void glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
  Assert(params, "Params was nullptr!");

  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *info = (ProgramInfo *)program;

  // Get parameter.
  switch (pname) {
  case GL_DELETE_STATUS:
    *params = (info->flags & PROGRAM_FLAG_DELETE) ? GL_TRUE : GL_FALSE;
    break;
  case GL_LINK_STATUS:
    *params = (info->flags & PROGRAM_FLAG_LINK_FAILED) ? GL_FALSE : GL_TRUE;
    break;
  case GL_VALIDATE_STATUS:
    *params = GL_TRUE;
    break;
  case GL_INFO_LOG_LENGTH:
    *params = 0;
    break;
  case GL_ATTACHED_SHADERS:
    if (ObjectIsShader(info->attachedVertex) &&
        ObjectIsShader(info->attachedGeometry))
      *params = 2;
    else if (ObjectIsShader(info->attachedVertex) ||
             ObjectIsShader(info->attachedGeometry))
      *params = 1;
    else
      *params = 0;
    break;
  case GL_ACTIVE_UNIFORMS:
    *params = NumActiveUniforms(info);
    break;
  case GL_ACTIVE_UNIFORM_MAX_LENGTH:
    *params = LenActiveUniforms(info);
    break;
    // TODO
  case GL_ACTIVE_ATTRIBUTES:
  case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
  default:
    SetError(GL_INVALID_ENUM);
  }
}

void glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
  Assert(params, "Params was nullptr!");

  if (!ObjectIsShader(shader)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ShaderInfo *info = (ShaderInfo *)shader;

  // Get parameter.
  switch (pname) {
  case GL_SHADER_TYPE:
    *params = (info->flags & SHADER_FLAG_GEOMETRY) ? GL_GEOMETRY_SHADER_PICA
                                                   : GL_VERTEX_SHADER;
    break;
  case GL_DELETE_STATUS:
    *params = (info->flags & SHADER_FLAG_DELETE) ? GL_TRUE : GL_FALSE;
    break;
  case GL_COMPILE_STATUS:
  case GL_INFO_LOG_LENGTH:
  case GL_SHADER_SOURCE_LENGTH:
    SetError(GL_INVALID_OPERATION);
    break;
  default:
    SetError(GL_INVALID_ENUM);
  }
}

GLboolean glIsProgram(GLuint program) {
  return ObjectIsProgram(program) ? GL_TRUE : GL_FALSE;
}

GLboolean glIsShader(GLuint shader) {
  return ObjectIsShader(shader) ? GL_TRUE : GL_FALSE;
}

void glLinkProgram(GLuint program) {
  if (!ObjectIsProgram(program)) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  ProgramInfo *pinfo = (ProgramInfo *)program;

  // Check for attached vertex shader (required).
  if (!ObjectIsShader(pinfo->attachedVertex)) {
    pinfo->flags |= PROGRAM_FLAG_LINK_FAILED;
    return;
  }

  // Check if vertex shader is new.
  if (pinfo->attachedVertex != pinfo->linkedVertex) {
    ShaderInfo *vsinfo = (ShaderInfo *)pinfo->attachedVertex;

    // Check load.
    if (!vsinfo->sharedData) {
      pinfo->flags |= PROGRAM_FLAG_LINK_FAILED;
      return;
    }

    // Unlink old vertex shader.
    if (ObjectIsShader(pinfo->linkedVertex))
      DecShaderRefc((ShaderInfo *)pinfo->linkedVertex);

    // Link new vertex shader.
    pinfo->flags |= PROGRAM_FLAG_UPDATE_VERTEX;
    pinfo->linkedVertex = pinfo->attachedVertex;
    ++vsinfo->refc;
  }

  // Check for attached geometry shader (optional).
  if (ObjectIsShader(pinfo->attachedGeometry) &&
      (pinfo->attachedGeometry != pinfo->linkedGeometry)) {
    ShaderInfo *gsinfo = (ShaderInfo *)pinfo->attachedGeometry;

    // Check load.
    if (!gsinfo->sharedData) {
      pinfo->flags |= PROGRAM_FLAG_LINK_FAILED;
      return;
    }

    // Unlink old geometry shader.
    if (ObjectIsShader(pinfo->linkedGeometry))
      DecShaderRefc((ShaderInfo *)pinfo->linkedGeometry);

    // Link new geometry shader.
    pinfo->flags |= PROGRAM_FLAG_UPDATE_GEOMETRY;
    pinfo->linkedGeometry = pinfo->attachedGeometry;
    ++gsinfo->refc;
  }

  pinfo->flags &= ~PROGRAM_FLAG_LINK_FAILED;
}

void glShaderBinary(GLsizei n, const GLuint *shaders, GLenum binaryformat,
                    const void *binary, GLsizei length) {
  Assert(shaders, "Shaders was nullptr!");
  Assert(binary, "Binary was nullptr!");

  const u8 *data = (u8 *)binary;
  const size_t size = length;
  size_t lastVertexIdx = (size_t)-1;
  size_t lastGeometryIdx = (size_t)-1;
  DVLB *dvlb = NULL;
  SharedShaderData *sharedData = NULL;

  if (binaryformat != GL_SHADER_BINARY_PICA) {
    SetError(GL_INVALID_ENUM);
    return;
  }

  if (n < 0 || length < 0) {
    SetError(GL_INVALID_VALUE);
    return;
  }

  // Do nothing if no shaders.
  if (!n)
    goto glShaderBinary_skip;

  // Parse DVLB.
  dvlb = ParseDVLB(data, size);
  if (!dvlb) {
    SetError(GL_OUT_OF_MEMORY);
    goto glShaderBinary_skip;
  }

  // Parse DVLP.
  const size_t dvlbSize = DVLB_MIN_SIZE + (dvlb->numOfDVLEs * 4);
  sharedData = ParseDVLP(data + dvlbSize, size - dvlbSize);
  if (!sharedData) {
    SetError(GL_OUT_OF_MEMORY);
    goto glShaderBinary_skip;
  }

  // Handle DVLEs.
  for (size_t i = 0; i < dvlb->numOfDVLEs; i++) {
    const u8 *pointerToDVLE = (u8 *)dvlb->DVLETable[i];
    const size_t maxSize = size - (size_t)(pointerToDVLE - data);

    // Lookup shader.
    DVLEInfo info;
    GetDVLEInfo(pointerToDVLE, maxSize, &info);
    const size_t index = LookupShader(
        shaders, n, (info.isGeometry ? lastGeometryIdx : lastVertexIdx),
        info.isGeometry);

    if (index == (info.isGeometry ? lastGeometryIdx : lastVertexIdx)) {
      SetError(GL_INVALID_OPERATION);
      goto glShaderBinary_skip;
    }

    // Build shader.
    ShaderInfo *shader = (ShaderInfo *)shaders[index];
    if (info.mergeOutmaps)
      shader->flags |= SHADER_FLAG_MERGE_OUTMAPS;
    else
      shader->flags &= ~SHADER_FLAG_MERGE_OUTMAPS;

    shader->codeEntrypoint = info.entrypoint;

    if (info.isGeometry)
      shader->gsMode = info.gsMode;

    GenerateOutmaps(&info, shader);

    // Make copy of symbol table.
    FreeMem(shader->symbolTable);
    shader->symbolTable = NULL;
    shader->sizeOfSymbolTable = 0;

    shader->symbolTable = (char *)AllocMem(info.sizeOfSymbolTable);
    if (!shader->symbolTable) {
      SetError(GL_OUT_OF_MEMORY);
      goto glShaderBinary_skip;
    }

    CopyMem(info.symbolTable, shader->symbolTable, info.sizeOfSymbolTable);
    shader->sizeOfSymbolTable = info.sizeOfSymbolTable;

    // Load uniforms, can only fail for memory issues.
    if (!LoadUniforms(&info, shader)) {
      SetError(GL_OUT_OF_MEMORY);
      goto glShaderBinary_skip;
    }

    // Set shared data.
    if (shader->sharedData)
      DecSharedDataRefc(shader->sharedData);

    shader->sharedData = sharedData;
    ++sharedData->refc;

    // Increment index.
    if (info.isGeometry)
      lastGeometryIdx = index;
    else
      lastVertexIdx = index;
  }

glShaderBinary_skip:
  // Free resources.
  if (sharedData && !sharedData->refc)
    FreeMem(sharedData);

  FreeMem(dvlb);
}

void glUseProgram(GLuint program) {
  if (!ObjectIsProgram(program) && program != GLASS_INVALID_OBJECT) {
    SetError(GL_INVALID_OPERATION);
    return;
  }

  CtxImpl *ctx = GetContext();

  // Check if already in use.
  if (ctx->currentProgram != program) {
    // Check if program is linked.
    if (ObjectIsProgram(program)) {
      ProgramInfo *info = (ProgramInfo *)program;
      if (info->flags & PROGRAM_FLAG_LINK_FAILED) {
        SetError(GL_INVALID_VALUE);
        return;
      }
    }

    // Remove program.
    if (ObjectIsProgram(ctx->currentProgram))
      FreeProgram((ProgramInfo *)ctx->currentProgram);

    // Set program.
    ctx->currentProgram = program;
    ctx->flags |= CONTEXT_FLAG_PROGRAM;
  }
}

// Stubs

void glCompileShader(GLuint shader) { SetError(GL_INVALID_OPERATION); }

void glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length,
                         GLchar *infoLog) {
  Assert(infoLog, "Info log was nullptr!");

  if (length)
    *length = 0;

  *infoLog = '\0';
}

void glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length,
                        GLchar *infoLog) {
  Assert(infoLog, "Info log was nullptr!");

  if (length)
    *length = 0;

  *infoLog = '\0';
}

void glGetShaderPrecisionFormat(GLenum shaderType, GLenum precisionType,
                                GLint *range, GLint *precision) {
  SetError(GL_INVALID_OPERATION);
}

void glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length,
                       GLchar *source) {
  SetError(GL_INVALID_OPERATION);
}

void glReleaseShaderCompiler(void) { SetError(GL_INVALID_OPERATION); }

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string,
                    const GLint *length) {
  SetError(GL_INVALID_OPERATION);
}

void glValidateProgram(GLuint program) {}