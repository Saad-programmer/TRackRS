// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * Minimal external-driver leak reproducer. Owns one SDL window and GL context
 * until explicit teardown; main-thread only. One clear/swap, no TrackRS code.
 * Compile with -fsanitize=address and run under Xvfb with llvmpipe.
 */
#include <SDL2/SDL.h>
#include <GL/glew.h>
int main() {
    if (SDL_Init(SDL_INIT_VIDEO)) return 1;
    auto* window = SDL_CreateWindow("probe", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
    if (!window) { SDL_Quit(); return 2; }
    auto context = SDL_GL_CreateContext(window);
    if (!context) { SDL_DestroyWindow(window); SDL_Quit(); return 3; }
    glewInit();
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
