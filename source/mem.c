#include "mem.h"

#include <stdlib.h>

// Globals

static glassAllocator g_Allocator = NULL;
static glassDeallocator g_Deallocator = NULL;

// Mem

void GLASS_mem_setAllocator(const glassAllocator allocator,
                            const glassDeallocator deallocator) {
  g_Allocator = allocator;
  g_Deallocator = deallocator;
}

void *GLASS_mem_alloc(const size_t size) {
  void *p = g_Allocator ? g_Allocator(size) : malloc(size);
  if (p)
    ZeroMem(p, size);
  return p;
}

void GLASS_mem_free(void *p) {
  if (p)
    g_Deallocator ? g_Deallocator(p) : free(p);
}