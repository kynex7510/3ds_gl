/**
 * context.h
 * Context state management.
 */
#ifndef _GLASS_CONTEXT_H
#define _GLASS_CONTEXT_H

#include "types.h"

// Initialize a context.
#define InitContext GLASS_context_initContext
void GLASS_context_initContext(CtxImpl *ctx);

// Finalize a context.
#define FiniContext GLASS_context_finiContext
void GLASS_context_finiContext(CtxImpl *ctx);

// Bind a context globally.
#define BindContext GLASS_context_bindContext
void GLASS_context_bindContext(CtxImpl *ctx);

// Get the global context.
#define GetContext GLASS_context_getContext
CtxImpl *GLASS_context_getContext(void);

// Applies any cached change to the global context.
#define UpdateContext GLASS_context_updateContext
CtxImpl *GLASS_context_updateContext(void);

// Set the last error.
#define SetError GLASS_context_setError
void GLASS_context_setError(GLenum error);

#endif /* _GLASS_CONTEXT_H */