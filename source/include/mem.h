/**
 * mem.h
 * Memory management and utilities.
 */
#ifndef _GLASS_MEM_H
#define _GLASS_MEM_H

#include "types.h"

#include <string.h>

// Virtual memory allocator.
#define AllocMem GLASS_mem_virtualAlloc
void *GLASS_mem_virtualAlloc(const size_t size);

// Virtual memory deallocator.
#define FreeMem GLASS_mem_virtualFree
void GLASS_mem_virtualFree(void *p);

// Linear memory allocator.
#define AllocLinear GLASS_mem_linearAlloc
void *GLASS_mem_linearAlloc(const size_t size);

// Linear memory deallocator.
#define FreeLinear GLASS_mem_linearFree
void GLASS_mem_linearFree(void *p);

// VRAM allocator.
#define AllocVRAM GLASS_mem_vramAlloc
void *GLASS_mem_vramAlloc(const size_t size, const vramAllocPos pos);

// VRAM deallocator.
#define FreeVRAM GLASS_mem_vramFree
void GLASS_mem_vramFree(void *p);

#define CopyMem(from, to, size)                                                \
  memcpy((void *)(to), (const void *)(from), (size))
#define CmpMem(p1, p2, size)                                                   \
  memcmp((const void *)(p1), (const void *)(p2), (size))
#define EqMem(p1, p2, size) !CmpMem((p1), (p2), (size))
#define SetMem(p, size, val) memset((void *)(p), (val), (size))
#define ZeroMem(p, size) SetMem((void *)(p), (size), 0x00)
#define ZeroVar(v) ZeroMem(&(v), sizeof((v)))

#endif /* _GLASS_MEM_H */