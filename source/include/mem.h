/**
 * mem.h
 * Memory management and utilities.
 */
#ifndef _GLASS_MEM_H
#define _GLASS_MEM_H

#include "types.h"

#include <string.h>

// Set allocator.
#define SetMemAlloc(alloc, dealloc) GLASS_mem_setAllocator((alloc), (dealloc))
void GLASS_mem_setAllocator(const glassAllocator allocator,
                            const glassDeallocator deallocator);

// Memory allocator.
#define AllocMem(size) GLASS_mem_alloc((size))
void *GLASS_mem_alloc(const size_t size);

// Memory deallocator.
#define FreeMem(p) GLASS_mem_free((uint8_t *)(p))
void GLASS_mem_free(void *p);

#define CopyMem(from, to, size)                                                \
  memcpy((void *)(to), (const void *)(from), (size))
#define CmpMem(p1, p2, size)                                                   \
  memcmp((const void *)(p1), (const void *)(p2), (size))
#define EqMem(p1, p2, size) !CmpMem((p1), (p2), (size))
#define SetMem(p, size, val) memset((void *)(p), (val), (size))
#define ZeroMem(p, size) SetMem((void *)(p), (size), 0x00)
#define ZeroVar(v) ZeroMem(&(v), sizeof((v)))

#endif /* _GLASS_MEM_H */