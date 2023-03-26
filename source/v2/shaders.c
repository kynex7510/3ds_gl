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
  uint32_t numOfDVLEs; // Number of DVLE entries.
  uint32_t *DVLETable; // Pointer to DVLE entries.
} DVLB;

typedef struct {
  u16 type; //  Constant type.
  u16 ID;   // Constant ID.
  union {
    u32 boolUniform; // Bool uniform value.
    struct {
      u8 x; // Int uniform X component.
      u8 y; // Int uniform Y component.
      u8 z; // Int uniform Z component.
      u8 w; // Int uniform W component.
    } intUniform;
    struct {
      u32 x; // Float uniform X component.
      u32 y; // Float uniform Y component.
      u32 z; // Float uniform Z component.
      u32 w; // Float uniform W component.
    } floatUniform;
  } data;
} DVLEConstEntry;

typedef DVLE_outEntry_s DVLEOutEntry;
typedef DVLE_geoShaderMode DVLEGSMode;

typedef struct {
  bool isGeometry;               // This DVLE is for a geometry shader.
  bool mergeOutmaps;             // Merge shader outmaps (geometry only).
  uint32_t entrypoint;           // Code entrypoint.
  DVLEGSMode gsMode;             // Geometry shader mode.
  DVLEConstEntry *constUniforms; // Constant uniform table.
  uint32_t numOfConstUniforms;   // Size of constant uniform table.
  DVLEOutEntry *outRegs;         // Output register table.
  uint32_t numOfOutRegs;         // Size of output register table.
  uint8_t *symbolTable;          // Symbol table.
  uint32_t sizeOfSymbolTable;    // Size of symbol table.
} DVLEInfo;

// Helpers

#define DecSharedDataRefc GLASS_shaders_decSharedDataRefc
static void GLASS_shaders_decSharedDataRefc(SharedShaderData *sharedData) {
  Assert(sharedData, "Shared shader data was nullptr!");

  if (sharedData->refc)
    --sharedData->refc;

  if (!sharedData->refc)
    FreeMem(sharedData);
}

#define DecShaderRefc GLASS_shaders_decShaderRefc
static void GLASS_shaders_decShaderRefc(ShaderInfo *info) {
  Assert(info, "Info was nullptr!");

  if (info->refc)
    --info->refc;

  if (!info->refc) {
    Assert(info->flags & SHADER_FLAG_DELETE,
           "Attempted to delete unflagged shader!");

    if (info->sharedData)
      DecSharedDataRefc(info->sharedData);

    FreeMem(info);
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

  if (info->linkedVertex) {
    ShaderInfo *vshad = (ShaderInfo *)info->linkedVertex;
    numOfActiveUniforms = vshad->numOfUniforms;
  }

  if (info->linkedGeometry) {
    ShaderInfo *gshad = (ShaderInfo *)info->linkedGeometry;
    numOfActiveUniforms += gshad->numOfUniforms;
  }

  return numOfActiveUniforms;
}

#define LenActiveUniforms GLASS_shaders_lenActiveUniforms
static size_t GLASS_shaders_lenActiveUniforms(ProgramInfo *info) {
  Assert(info, "Info was nullptr!");

  size_t lenOfActiveUniforms = 0;

  if (info->linkedVertex) {
    ShaderInfo *vshad = (ShaderInfo *)info->linkedVertex;
    for (size_t i = 0; i < vshad->numOfUniforms; i++) {
      const UniformInfo *uni = &vshad->activeUniforms[i];
      if (!vshad->activeUniforms[i].symbol)
    }
  }
}

#define LookupShader GLASS_shaders_lookupShader
static size_t GLASS_shaders_lookupShader(const GLuint *shaders,
                                         const size_t maxShaders,
                                         const size_t index,
                                         const bool isGeometry) {
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
DVLB *GLASS_shaders_parseDVLB(const uint8_t *data, const size_t size) {
  Assert(data, "Data was nullptr!");
  Assert(size > DVLB_MIN_SIZE, "Invalid DVLB size!");
  Assert(EqMem(data, DVLB_MAGIC, 4), "Invalid DVLB header!");

  // Read number of DVLEs.
  uint32_t numOfDVLEs = 0;
  CopyMem(data + 0x04, &numOfDVLEs, 4);
  Assert((DVLB_MIN_SIZE + (numOfDVLEs * 4)) <= size, "DVLE table OOB!");

  // Allocate DVLB.
  const size_t sizeOfDVLB = sizeof(DVLB) + (numOfDVLEs * 4);
  DVLB *dvlb = (DVLB *)AllocMem(sizeOfDVLB);

  if (dvlb) {
    dvlb->numOfDVLEs = numOfDVLEs;
    dvlb->DVLETable = (uint32_t *)((uint8_t *)dvlb + sizeof(DVLB));

    // Fill table with offsets.
    for (size_t i = 0; i < numOfDVLEs; i++) {
      dvlb->DVLETable[i] = *(uint32_t *)(data + DVLB_MIN_SIZE + (4 * i));
      Assert(dvlb->DVLETable[i] <= size, "DVLE offset OOB!");
      // Relocation.
      dvlb->DVLETable[i] += (uint32_t)data;
    }
  }

  return dvlb;
}

#define ParseDVLP GLASS_shaders_parseDVLP
SharedShaderData *GLASS_shaders_parseDVLP(const uint8_t *data,
                                          const size_t size) {
  Assert(data, "Data was nullptr!");
  Assert(size > DVLP_MIN_SIZE, "Invalid DVLP size!");
  Assert(EqMem(data, DVLP_MAGIC, 4), "Invalid DVLP header!");

  // Read offsets.
  uint32_t offsetToBlob = 0;
  uint32_t offsetToOpDescs = 0;
  CopyMem(data + 0x08, &offsetToBlob, 4);
  CopyMem(data + 0x10, &offsetToOpDescs, 4);
  Assert(offsetToBlob < size, "DVLP blob start offset OOB!");
  Assert(offsetToOpDescs < size, "DVLP opdescs start offset OOB!");

  // Read num params.
  uint32_t numOfCodeWords = 0;
  uint32_t numOfOpDescs = 0;
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
        (uint32_t *)((uint8_t *)sharedData + sizeof(SharedShaderData));
    sharedData->numOfCodeWords = numOfCodeWords;
    sharedData->opDescs = sharedData->binaryCode + sharedData->numOfCodeWords;
    sharedData->numOfOpDescs = numOfOpDescs;

    // Relocation.
    offsetToBlob += (uint32_t)data;
    offsetToOpDescs += (uint32_t)data;

    // Read binary code.
    CopyMem(offsetToBlob, sharedData->binaryCode,
            sharedData->numOfCodeWords * 4);

    // Read op descs.
    for (size_t i = 0; i < sharedData->numOfOpDescs; i++)
      sharedData->opDescs[i] = ((uint32_t *)offsetToOpDescs)[i * 2];
  }

  return sharedData;
}

#define GetDVLEInfo GLASS_shaders_getDVLEInfo
void GLASS_shaders_getDVLEInfo(const uint8_t *data, const size_t size,
                               DVLEInfo *out) {
  Assert(data, "Data was nullptr!");
  Assert(out, "Out was nullptr!");
  Assert(size > DVLE_MIN_SIZE, "Invalid DVLE size!");
  Assert(EqMem(data, DVLE_MAGIC, 4), "Invalid DVLE header!");

  // Get info.
  uint8_t flags = 0;
  uint8_t mergeOutmaps = 0;
  uint8_t gsMode = 0;
  uint32_t offsetToConstTable = 0;
  uint32_t offsetToOutTable = 0;
  uint32_t offsetToSymbolTable = 0;
  CopyMem(data + 0x06, &flags, 1);
  CopyMem(data + 0x07, &mergeOutmaps, 1);
  CopyMem(data + 0x08, &out->entrypoint, 4);
  CopyMem(data + 0x14, &gsMode, 1);
  CopyMem(data + 0x18, &offsetToConstTable, 4);
  CopyMem(data + 0x1C, &out->numOfConstUniforms, 4);
  CopyMem(data + 0x28, &offsetToOutTable, 4);
  CopyMem(data + 0x2C, &out->numOfOutRegs, 4);
  CopyMem(data + 0x38, &offsetToSymbolTable, 4);
  copyMem(data + 0x3C, &out->sizeOfSymbolTable, 4);
  Assert(offsetToConstTable < size, "DVLE const table start offset OOB!");
  Assert(offsetToOutTable < size, "DVLE output table start offset OOB!");
  Assert(offsetToSymbolTable < size, "DVLE symbol table start offset OOB!");
  Assert((offsetToConstTable + (out->numOfConstUniforms * 20)) <= size,
         "DVLE const table end offset OOB!");
  Assert((offsetToOutTable + (out->numOfOutRegs * 8)) <= size,
         "DVLE output table end offset OOB!");
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
  offsetToConstTable += (uint32_t)data;
  offsetToOutTable += (uint32_t)data;
  offsetToSymbolTable += (uint32_t)data;

  // Set table pointers.
  out->constUniforms = (DVLEConstEntry *)offsetToConstTable;
  out->outRegs = (DVLE_outEntry_s *)offsetToOutTable;
  out->symbolTable = (uint8_t *)offsetToSymbolTable;
}

#define GenerateOutmaps GLASS_shaders_generateOutmaps
void GLASS_shaders_generateOutmaps(const DVLEInfo *info, ShaderInfo *out) {
  Assert(info, "Info was nullptr!");
  Assert(out, "Out was nullptr!");

  bool useTexcoords = false;
  out->outMask = 0;
  out->outTotal = 0;
  out->outClock = 0;
  SetMem(out->outSems, sizeof(out->outSems), 0x1F);

  for (size_t i = 0; i < info->numOfOutRegs; i++) {
    const DVLE_outEntry_s *entry = &info->outRegs[i];
    uint8_t sem = 0x1F;
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

  // Reset.
  FreeMem(out->symbolTable);
  out->symbolTable = NULL;
  out->sizeOfSymbolTable = 0;
  FreeMem(out->uniformInfo);
  out->uniformInfo = NULL;
  out->numOfUniforms = 0;
  out->boolUniforms = 0;
  ZeroVar(out->intUniforms);

  // Copy symbol table.
  out->symbolTable = (uint8_t *)AllocMem(info->sizeOfSymbolTable);
  if (!out->symbolTable)
    goto loadUniforms_error;

  CopyMem(info->symbolTable, out->symbolTable, info->sizeOfSymbolTable);
  out->sizeOfSymbolTable = info->sizeOfSymbolTable;

loadUniforms_error:
  if (out->symbolTable) {
    FreeMem(out->symbolTable);
    out->symbolTable = NULL;
  }

  out->sizeOfSymbolTable = 0;

  // Load bool and int uniforms.
  size_t maxOfFloatUniforms = 0;
  for (size_t i = 0; i < info->numOfConstUniforms; i++) {
    const DVLEConstEntry *entry = &info->constUniforms[i];

    switch (entry->type) {
    case DVLE_CONST_BOOL:
      SetShaderBool(out, entry->ID, entry->data.boolUniform & 1);
      break;
    case DVLE_CONST_INT:
      u32 components[4];
      components[0] = entry->data.intUniform.x;
      components[1] = entry->data.intUniform.y;
      components[2] = entry->data.intUniform.z;
      components[3] = entry->data.intUniform.w;
      SetShaderInts(out, entry->ID, components);
      break;
    case DVLE_CONST_FLOAT:
      maxOfFloatUniforms++;
      break;
    default:
      Unreachable("Unknown constant uniform type!");
    }
  }

  if (maxOfFloatUniforms) {
    // Allocate float data.
    out->floatUniforms =
        (uint32_t *)AllocMem(maxOfFloatUniforms * (sizeof(uint32_t) * 3));
    if (!out->floatUniforms)
      return false;

    out->floatUniformsMD =
        (uint8_t *)AllocMem(maxOfFloatUniforms * sizeof(uint8_t));
    if (!out->floatUniformsMD) {
      FreeMem(out->floatUniforms);
      out->floatUniforms = NULL;
      return false;
    }

    ZeroMem(out->floatUniforms, maxOfFloatUniforms * (sizeof(uint32_t) * 3));
    ZeroMem(out->floatUniformsMD, maxOfFloatUniforms * sizeof(uint8_t));

    // Load float uniforms.
    for (size_t i = 0; i < info->numOfConstUniforms; i++) {
      const DVLEConstEntry *entry = &info->constUniforms[i];
      if (entry->type != DVLE_CONST_FLOAT)
        continue;

      Assert(out->numOfFloatUniforms < maxOfFloatUniforms,
             "Constant uniform OOB!");
      out->floatUniformsMD[out->numOfFloatUniforms++] = entry->ID;

      float components[4];
      components[0] = f24tof32(entry->data.floatUniform.x);
      components[1] = f24tof32(entry->data.floatUniform.y);
      components[2] = f24tof32(entry->data.floatUniform.z);
      components[3] = f24tof32(entry->data.floatUniform.w);
      SetShaderFloats(out, entry->ID, components);
    }
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
    if (pinfo->attachedGeometry != GLASS_INVALID_OBJECT) {
      SetError(GL_INVALID_OPERATION);
      return;
    }

    pinfo->attachedGeometry = shader;
  } else {
    if (pinfo->attachedVertex != GLASS_INVALID_OBJECT) {
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

  if (name != GLASS_INVALID_OBJECT) {
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
  uint16_t flags = 0;

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

  if (name != GLASS_INVALID_OBJECT) {
    ShaderInfo *info = (ShaderInfo *)name;
    info->sharedData = NULL;
    info->codeEntrypoint = 0;
    info->gsMode = 0;
    info->outMask = 0;
    info->boolUniforms = 0;
    ZeroVar(info->intUniforms);
    info->intUniformsMD = 0;
    info->numOfFloatUniforms = 0;
    info->floatUniforms = NULL;
    info->floatUniformsMD = NULL;
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

  ProgramInfo *info = (ProgramInfo *)program;

  // Get shaders.
  if (info->attachedVertex != GLASS_INVALID_OBJECT) {
    Assert(ObjectIsShader(info->attachedVertex), "Invalid vertex shader!");
    shaders[shaderCount++] = info->attachedVertex;
  }

  if ((info->attachedGeometry != GLASS_INVALID_OBJECT) && maxCount > 1) {
    Assert(ObjectIsShader(info->attachedGeometry), "Invalid geometry shader!");
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
    if ((info->attachedVertex != GLASS_INVALID_OBJECT) &&
        (info->attachedGeometry != GLASS_INVALID_OBJECT))
      *params = 2;
    else if ((info->attachedVertex != GLASS_INVALID_OBJECT) ||
             (info->attachedGeometry != GLASS_INVALID_OBJECT))
      *params = 1;
    else
      *params = 0;
    break;
  case GL_ACTIVE_UNIFORMS:
    *params = NumActiveUniforms(info);
    break;
  case GL_ACTIVE_UNIFORMS_MAX_LENGTH:
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

  const uint8_t *data = (uint8_t *)binary;
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
    const uint8_t *pointerToDVLE = (uint8_t *)dvlb->DVLETable[i];
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
    shader->gsMode = info.gsMode;

    GenerateOutmaps(&info, shader);

    // This can only fail for memory issues.
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
  if (sharedData) {
    if (!sharedData->refc)
      FreeMem(sharedData);
  }

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
    if (program != GLASS_INVALID_OBJECT) {
      ProgramInfo *info = (ProgramInfo *)program;
      if (info->flags & PROGRAM_FLAG_LINK_FAILED) {
        SetError(GL_INVALID_VALUE);
        return;
      }
    }

    // Remove program.
    if (ctx->currentProgram != GLASS_INVALID_OBJECT)
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