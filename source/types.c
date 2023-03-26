#include "utility.h"

// Types

GLuint GLASS_types_createObject(const uint32_t type) {
  size_t objSize = 0;

  switch (type) {
  case GLASS_BUFFER_TYPE:
    objSize = sizeof(BufferInfo);
    break;
    // case GLASS_TEXTURE_TYPE:
  case GLASS_PROGRAM_TYPE:
    objSize = sizeof(ProgramInfo);
    break;
  case GLASS_SHADER_TYPE:
    objSize = sizeof(ShaderInfo);
    break;
  case GLASS_FRAMEBUFFER_TYPE:
    objSize = sizeof(FramebufferInfo);
    break;
  case GLASS_RENDERBUFFER_TYPE:
    objSize = sizeof(RenderbufferInfo);
    break;
  default:
    return GLASS_INVALID_OBJECT;
  }

  uint32_t *obj = AllocMem(objSize);

  if (obj) {
    *obj = type;
    return (GLuint)obj;
  }

  return GLASS_INVALID_OBJECT;
}

bool GLASS_types_checkObjectType(const GLuint obj, const uint32_t type) {
  if (obj != GLASS_INVALID_OBJECT) {
#ifndef NDEBUG
    MemInfo minfo;
    PageInfo pinfo;
    svcQueryProcessMemory(&minfo, &pinfo, CUR_PROCESS_HANDLE, obj);
    Assert(minfo.perm & MEMPERM_READ,
           "Attempted to read invalid memory location!");
#endif
    return *(uint32_t *)obj == type;
  }

  return false;
}