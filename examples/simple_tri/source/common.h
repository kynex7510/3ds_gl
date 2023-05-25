#ifndef _COMMON_H
#define _COMMON_H

#include "GLASS.h"
#include "gl2.h"

#include <3ds.h>
#include <citro3d.h>

#if defined(COMMON_HAS_VSHAD)
#include "vshader_shbin.h"
#endif

#include <stdio.h>
#include <string.h>

#define GLCheck2(line, ...)                                                    \
  __VA_ARGS__;                                                                 \
  checkError(line);

#define GLCheck(...) GLCheck2(__LINE__, __VA_ARGS__)

static const char *stringifyError(const GLenum error) {
  switch (error) {
  case GL_INVALID_ENUM:
    return "invalid enum";
  case GL_INVALID_VALUE:
    return "invalid value";
  case GL_INVALID_OPERATION:
    return "invalid operation";
  case GL_OUT_OF_MEMORY:
    return "out of memory";
  default:
    return "unknown";
  }
}

static void breakWithError(const char *s) {
  svcOutputDebugString(s, strlen(s));
  svcBreak(USERBREAK_PANIC);
}

static void checkError(const size_t line) {
  char buffer[80];
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    sprintf(buffer, "ERROR: \"%s\" (%04x) at line %u\n", stringifyError(error),
            error, line);
    breakWithError(buffer);
  }
}

#if defined(COMMON_HAS_CONSOLE)

static size_t g_UsedMem = 0;
static size_t g_UsedLinearMem = 0;
static size_t g_UsedVRAM = 0;

static void printNumBytes(const size_t size) {
  if (size > 1 * 1000 * 1000) {
    printf("%.3f MB\n", size / (1.0f * 1000 * 1000));
  } else if (size > 1 * 1000) {
    printf("%.3f KB\n", size / (1.0f * 1000));
  } else {
    printf("%u bytes\n", size);
  }
}

static void refreshDebugStats(void) {
  consoleClear();
  printf("MEMORY USAGE\n");

  printf("- Virtual memory: ");
  printNumBytes(g_UsedMem);

  printf("- Linear memory: ");
  printNumBytes(g_UsedLinearMem);

  printf("- VRAM: ");
  printNumBytes(g_UsedVRAM);

  printf("- Total: ");
  printNumBytes(g_UsedMem + g_UsedLinearMem + g_UsedVRAM);
}

void GLASS_virtualAllocHook(const void *p, const size_t size) {
  g_UsedMem += size;
  refreshDebugStats();
}

void GLASS_virtualFreeHook(const void *p, const size_t size) {
  g_UsedMem -= size;
  refreshDebugStats();
}

void GLASS_linearAllocHook(const void *p, const size_t size) {
  g_UsedLinearMem += size;
  refreshDebugStats();
}

void GLASS_linearFreeHook(const void *p, const size_t size) {
  g_UsedLinearMem -= size;
  refreshDebugStats();
}

void GLASS_vramAllocHook(const void *p, const size_t size) {
  g_UsedVRAM += size;
  refreshDebugStats();
}

void GLASS_vramFreeHook(const void *p, const size_t size) {
  g_UsedVRAM -= size;
  refreshDebugStats();
}

#endif

static GLuint setupShaderProgram(void) {
  GLCheck(GLuint sprog = glCreateProgram());

#if defined(COMMON_HAS_VSHAD)
  GLCheck(GLuint vshad = glCreateShader(GL_VERTEX_SHADER));
  GLCheck(glShaderBinary(1, &vshad, GL_SHADER_BINARY_PICA, vshader_shbin,
                         vshader_shbin_size));
  GLCheck(glAttachShader(sprog, vshad));
  GLCheck(glDeleteShader(vshad));
#endif

#if defined(COMMON_HAS_GSHAD)
  GLCheck(GLuint gshad = glCreateShader(GL_GEOMETRY_SHADER_PICA));
  GLCheck(glShaderBinary(1, &gshad, GL_SHADER_BINARY_PICA, gshader_shbin,
                         gshader_shbin_size));
  GLCheck(glAttachShader(sprog, gshad));
  GLCheck(glDeleteShader(gshad));
#endif

  GLCheck(glLinkProgram(sprog));
  GLCheck(glUseProgram(sprog));
  GLCheck(glDeleteProgram(sprog));
  return sprog;
}

static glassCtx *initTopScreen(void) {
  // Create context.
  glassCtx *ctx = glassCreateContext();
  ctx->targetScreen = GFX_TOP;
  ctx->targetSide = GFX_LEFT;
  glassBindContext(ctx);

  // Setup framebuffer.
  GLuint fb, rb;

  GLCheck(glGenFramebuffers(1, &fb));
  GLCheck(glGenRenderbuffers(1, &rb));
  GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, fb));
  GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, rb));
  GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 400, 240));
  GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, rb));

  // Set clear color.
  GLCheck(glClearColor(0.0, 0.0, 0.0, 1.0));

  return ctx;
}

#if !defined(COMMON_HAS_CONSOLE)
static glassCtx *initBottomScreen(void) {
  // Create context.
  glassCtx *ctx = glassCreateContext();
  ctx->targetScreen = GFX_BOTTOM;
  ctx->targetSide = GFX_LEFT;
  glassBindContext(ctx);

  // Setup framebuffer.
  GLuint fb, rb;

  GLCheck(glGenFramebuffers(1, &fb));
  GLCheck(glGenRenderbuffers(1, &rb));
  GLCheck(glBindFramebuffer(GL_FRAMEBUFFER, fb));
  GLCheck(glBindRenderbuffer(GL_RENDERBUFFER, rb));
  GLCheck(glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 320, 240));
  GLCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                    GL_RENDERBUFFER, rb));

  // Set clear color.
  GLCheck(glClearColor(0.0, 0.0, 0.0, 1.0));
  return ctx;
}
#endif

static void initCommon(glassCtx **topCtx, glassCtx **bottomCtx) {
  gfxInit(GSP_RGB565_OES, GSP_RGB565_OES, false);

#if defined(COMMON_HAS_CONSOLE)
  consoleInit(GFX_BOTTOM, NULL);
#endif

  if (topCtx)
    *topCtx = initTopScreen();

#if !defined(COMMON_HAS_CONSOLE)
  if (bottomCtx)
    *bottomCtx = initBottomScreen();
#else
  refreshDebugStats();
#endif
}

static void finiCommon(glassCtx *topCtx, glassCtx *bottomCtx) {
  if (topCtx)
    glassDestroyContext(topCtx);

  if (bottomCtx)
    glassDestroyContext(bottomCtx);

  gfxExit();
}

#endif /* _COMMON_H */