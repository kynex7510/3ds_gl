#define COMMON_HAS_CONSOLE
#define COMMON_HAS_VSHAD

#include "common.h"

#include <stdlib.h>

typedef struct {
  float x, y, z;
} position;

// Main

int main() {
  glassCtx *ctx;

  // Initialization.
  initCommon(&ctx, NULL);
  GLCheck(glViewport(0, 0, 400, 240));
  GLCheck(glClearColor(0.40625f, 0.6875f, 0.84375f, 1.0f));

  // Setup program.
  GLint prog = setupShaderProgram();

  // Setup uniform.
  C3D_Mtx projection = {};
  GLCheck(GLint projLoc = glGetUniformLocation(prog, "projection"));
  Mtx_OrthoTilt(&projection, 0.0, 400.0, 0.0, 240.0, 0.0, 1.0, true);
  GLCheck(glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection.m));

  // Setup attributes.
  GLuint vbo = 0;
  position vertices[3] = {
      {200.0f, 200.0f, 0.5f}, // Top
      {100.0f, 40.0f, 0.5f},  // Left
      {300.0f, 40.0f, 0.5f},  // Right
  };

  GLCheck(glGenBuffers(1, &vbo));
  GLCheck(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GLCheck(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                       GL_STATIC_DRAW));

  // Set position attribute.
  GLCheck(
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(position), NULL));
  GLCheck(glEnableVertexAttribArray(0));

  // Set color attribute.
  GLCheck(glVertexAttrib3f(1, 1.0f, 1.0f, 1.0f));
  GLCheck(glEnableVertexAttribArray(1));

  // Main loop.
  while (aptMainLoop()) {
    hidScanInput();

    // Respond to user input.
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break; // break in order to return to hbmenu.

    // Draw things.
    GLCheck(glClear(GL_COLOR_BUFFER_BIT));
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glassSwapBuffers();
    gspWaitForVBlank();
  }

  // Finalization.
  finiCommon(ctx, NULL);
  return 0;
}