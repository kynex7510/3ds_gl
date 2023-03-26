/**
 * get.h
 * Get* utilities.
 */
#ifndef _GLASS_GET_H
#define _GLASS_GET_H

#include "types.h"

#define GLASS_GET_MAX_PARAMS 16
#define GLASS_GET_FAILED ((size_t)-1)

// Get

#define GetBools GLASS_get_getBools
size_t GLASS_get_getBools(const GLenum pname, GLboolean *params);

#define GetFloats GLASS_get_getFloats
size_t GLASS_get_getFloats(const GLenum pname, GLfloat *params);

#define GetInts GLASS_get_getInts
size_t GLASS_get_getInts(const GLenum pname, GLint *params);

#define CastFloatAsInt GLASS_get_castFloatAsInt
GLint GLASS_get_castFloatAsInt(const GLenum pname, const GLfloat value,
                               const size_t index);

#endif /* _GLASS_GET_H */