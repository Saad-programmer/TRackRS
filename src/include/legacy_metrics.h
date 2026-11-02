// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Opt-in measurement of the compatibility renderer, not a replacement renderer.
 * Main-thread only; GL scopes must die before their context. A scope owns two
 * timestamp queries, handed to the frame collector to avoid stalls per scope.
 * Immediate-mode counters count submitted primitives (including degenerates).
 */
#include "validation.h"
#include <glad/gl.h>
#include <SDL2/SDL.h>
namespace validation {
inline std::uint64_t triangleCount(GLenum mode, unsigned n) {
    switch (mode) {
    case GL_TRIANGLES: return n / 3;
    case GL_TRIANGLE_STRIP: case GL_TRIANGLE_FAN: case GL_POLYGON: return n > 2 ? n - 2 : 0;
    case GL_QUADS: return n / 4 * 2;
    case GL_QUAD_STRIP: return n >= 4 ? (n / 2 - 1) * 2 : 0;
    default: return 0;
    }
}
inline void begin(GLenum mode) {
    if (run.capture) { ++run.frameDraws; run.primitive = mode; run.vertices = 0; }
    glBegin(mode);
}
inline void end() {
    glEnd();
    if (run.capture) run.frameTriangles += triangleCount(run.primitive, run.vertices);
}
inline void vertex2(GLfloat x, GLfloat y) {
    if (run.capture) ++run.vertices;
    glVertex2f(x, y);
}
inline void vertex3(GLfloat x, GLfloat y, GLfloat z) {
    if (run.capture) ++run.vertices;
    glVertex3f(x, y, z);
}
inline void vertex3v(const GLfloat* p) {
    if (run.capture) ++run.vertices;
    glVertex3fv(p);
}
inline void drawRange(GLenum mode, GLuint start, GLuint last, GLsizei count, GLenum type, const void* indices) {
    if (run.capture) {
        ++run.frameDraws;
        run.frameTriangles += triangleCount(mode, static_cast<unsigned>(count));
    }
    glDrawRangeElements(mode, start, last, count, type, indices);
}
class PassScope {
    Pass* pass = nullptr;
    Uint64 beginCpu = 0;
    GLuint queries[2] = {0, 0};
public:
    explicit PassScope(const char* name) {
        if (!run.capture) return;
        pass = &run.passes.at(name);
        beginCpu = SDL_GetPerformanceCounter();
        if ((GLAD_GL_VERSION_3_3 || GLAD_GL_ARB_timer_query)) {
            glGenQueries(2, queries);
            glQueryCounter(queries[0], GL_TIMESTAMP);
        }
    }
    ~PassScope() {
        if (!pass) return;
        const auto endCpu = SDL_GetPerformanceCounter();
        if (queries[0]) {
            glQueryCounter(queries[1], GL_TIMESTAMP);
            run.queries.push_back({pass, queries[0], queries[1]});
        }
        pass->cpu.push_back(1000.0 * static_cast<double>(endCpu - beginCpu) /
            static_cast<double>(SDL_GetPerformanceFrequency()));
    }
    PassScope(const PassScope&) = delete;
    PassScope& operator=(const PassScope&) = delete;
};
}
// GLAD aliases are replaced only after the forwarders bind actual entry points.
#undef glBegin
#undef glEnd
#undef glVertex2f
#undef glVertex3f
#undef glVertex3fv
#undef glDrawRangeElements
#define glBegin validation::begin
#define glEnd validation::end
#define glVertex2f validation::vertex2
#define glVertex3f validation::vertex3
#define glVertex3fv validation::vertex3v
#define glDrawRangeElements validation::drawRange
