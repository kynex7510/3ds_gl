#define COMMON_HAS_CONSOLE
#define COMMON_HAS_VSHAD

#include "common.h"

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

int main() {
  glassCtx *ctx;

  // Initialization.
  initCommon(&ctx, NULL);

  // Setup program.
  GLuint prog = setupShaderProgram();
  // glGetUniformLocation(prog, "projection");

  // Setup attributes.
  GLCheck(glEnableVertexAttribArray(0));
  GLCheck(glVertexAttrib4f(0, 1.0f, 0.0f, 0.0f, 1.0f));

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