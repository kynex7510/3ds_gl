/**
 * GLASS.h
 * Library management API.
 */
#ifndef _GLASS_H
#define _GLASS_H

#include <3ds.h>

// Public context structure.
typedef struct {
  gfxScreen_t targetScreen;        // Target screen for drawing (top or bottom).
  gfx3dSide_t targetSide;          // Target side for drawing (left or right).
  GX_TRANSFER_SCALE transferScale; // Anti-aliasing.
} glassCtx;

/**
 * Create and initialize a context. Return NULL on failure. Required for any
 * draw operation. Created context does *not* get automatically bound.
 */
glassCtx *glassCreateContext(void);

/**
 * Destroy context. If bound it gets automatically unbound. Context must be
 * valid (eg. non null).
 */
void glassDestroyContext(glassCtx *ctx);

/**
 * Bind global context. Any subsequent draw call will refer to this context.
 * Pass NULL to unbind the context. Unbinding a context will *not* flush any
 * draw operation. Binding a context will flag an update request, unless the
 * context is already bound, or was the last bound context before NULL.
 */
void glassBindContext(glassCtx *ctx);

/**
 * Flushes any pending draw operation, then swap the screen buffer.
 */
void glassSwapBuffers(void);

#endif /* _GLASS_H */