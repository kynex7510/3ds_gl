#define COMMON_HAS_CONSOLE
#define COMMON_HAS_VSHAD

#include "common.h"

// Citro3D code

typedef union {
  struct {
    float w; ///< W-component
    float z; ///< Z-component
    float y; ///< Y-component
    float x; ///< X-component
  };
  struct {
    float r; ///< Real component
    float k; ///< K-component
    float j; ///< J-component
    float i; ///< I-component
  };
  float c[4];
} C3D_FVec;

typedef union {
  C3D_FVec r[4];
  float m[4 * 4];
} C3D_Mtx;

static inline void Mtx_Zeros(C3D_Mtx *out) { memset(out, 0, sizeof(*out)); }

static void Mtx_OrthoTilt(C3D_Mtx *mtx, float left, float right, float bottom,
                          float top, float near, float far, bool isLeftHanded) {
  Mtx_Zeros(mtx);

  mtx->r[0].y = 2.0f / (top - bottom);
  mtx->r[0].w = (bottom + top) / (bottom - top);
  mtx->r[1].x = 2.0f / (left - right);
  mtx->r[1].w = (left + right) / (right - left);
  if (isLeftHanded)
    mtx->r[2].z = 1.0f / (far - near);
  else
    mtx->r[2].z = 1.0f / (near - far);
  mtx->r[2].w = 0.5f * (near + far) / (near - far) - 0.5f;
  mtx->r[3].w = 1.0f;
}

// Helpers

static void render(const u32 keys) {
  // Change clear color based on keys.
  if (keys & KEY_A) {
    GLCheck(glClearColor(1.0f, 0.0f, 0.0f, 1.0f));
  }

  if (keys & KEY_B) {
    GLCheck(glClearColor(0.0f, 1.0f, 0.0f, 1.0f));
  }

  if (keys & KEY_X) {
    GLCheck(glClearColor(0.0f, 0.0f, 1.0f, 1.0f));
  }

  if (keys & KEY_Y) {
    GLCheck(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
  }

  // Clear screen.
  GLCheck(glClear(GL_COLOR_BUFFER_BIT));
}

// Main

int main() {
  glassCtx *ctx;

  // Initialization.
  initCommon(&ctx, NULL);

  // Setup program.
  GLint prog = setupShaderProgram();

  // Setup uniform.
  C3D_Mtx projection = {};
  GLCheck(GLint projLoc = glGetUniformLocation(prog, "projection"));
  Mtx_OrthoTilt(&projection, 0.0, 400.0, 0.0, 240.0, 0.0, 1.0, true);
  GLCheck(glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.m));

  // Setup attributes.
  GLCheck(glVertexAttrib3f(0, 200.0f, 200.0f, 0.5f));
  GLCheck(glVertexAttrib3f(1, 100.0f, 40.0f, 0.5f));
  GLCheck(glVertexAttrib3f(2, 300.0f, 40.0f, 0.5f));

  GLCheck(glEnableVertexAttribArray(0));
  GLCheck(glEnableVertexAttribArray(1));
  GLCheck(glEnableVertexAttribArray(2));

  // Main loop.
  while (aptMainLoop()) {
    hidScanInput();

    // Respond to user input.
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break; // break in order to return to hbmenu.

    // Draw things.
    render(kDown);
    glassSwapBuffers();
    gspWaitForVBlank();
  }

  // Finalization.
  finiCommon(ctx, NULL);
  return 0;
}