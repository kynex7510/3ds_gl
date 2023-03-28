#include "mem.h"

#include <malloc.h>

#define WEAK __attribute__((weak))

// Default

WEAK void *GLASS_virtualAlloc(const size_t size) { return malloc(size); }
WEAK void GLASS_virtualFree(void *p) { free(p); }
WEAK void *GLASS_linearAlloc(const size_t size) { return linearAlloc(size); }
WEAK void GLASS_linearFree(void *p) { linearFree(p); }

WEAK void *GLASS_vramAlloc(const size_t size, const vramAllocPos pos) {
  return vramAllocAt(size, pos);
}

WEAK void GLASS_vramFree(void *p) { vramFree(p); }

// Hooks

#ifndef NDEBUG

WEAK void GLASS_virtualAllocHook(const void *p, const size_t size) {}
WEAK void GLASS_virtualFreeHook(const void *p, const size_t size) {}
WEAK void GLASS_linearAllocHook(const void *p, const size_t size) {}
WEAK void GLASS_linearFreeHook(const void *p, const size_t size) {}
WEAK void GLASS_vramAllocHook(const void *p, const size_t size) {}
WEAK void GLASS_vramFreeHook(const void *p, const size_t size) {}

#endif // NDEBUG

// Mem

void *GLASS_mem_virtualAlloc(const size_t size) {
  void *p = GLASS_virtualAlloc(size);
  if (p) {
    ZeroMem(p, size);
#ifndef NDEBUG
    GLASS_virtualAllocHook(p, malloc_usable_size(p));
#endif
  }

  return p;
}

void GLASS_mem_virtualFree(void *p) {
  if (p) {
#ifndef NDEBUG
    const size_t size = malloc_usable_size(p);
#endif
    GLASS_virtualFree(p);
#ifndef NDEBUG
    GLASS_virtualFreeHook(p, size);
#endif
  }
}

void *GLASS_mem_linearAlloc(const size_t size) {
  void *p = GLASS_linearAlloc(size);
  if (p) {
    ZeroMem(p, size);
#ifndef NDEBUG
    GLASS_linearAllocHook(p, size);
#endif
  }

  return p;
}

void GLASS_mem_linearFree(void *p) {
  if (p) {
#ifndef NDEBUG
    const size_t size = linearGetSize(p);
#endif
    GLASS_linearFree(p);
#ifndef NDEBUG
    GLASS_linearFreeHook(p, size);
#endif
  }
}

void *GLASS_mem_vramAlloc(const size_t size, const vramAllocPos pos) {
  void *p = GLASS_vramAlloc(size, pos);
  if (p) {
#ifndef NDEBUG
    GLASS_vramAllocHook(p, size);
#endif
  }

  return p;
}

void GLASS_mem_vramFree(void *p) {
  if (p) {
#ifndef NDEBUG
    const size_t size = vramGetSize(p);
#endif
    GLASS_vramFree(p);
#ifndef NDEBUG
    GLASS_vramFreeHook(p, size);
#endif
  }
}