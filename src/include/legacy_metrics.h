// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
/**
 * Opt-in CPU/GPU pass measurements for the core renderer.
 * Main-thread only; GL scopes must die before their context. A scope owns two
 * timestamp queries, handed to the frame collector to avoid stalls per scope.
 * Actual GPU draw/triangle counters are recorded by engine/render/basic.cpp.
 */
#include "validation.h"
#include "engine/render/basic.h"
#include <glad/gl.h>
#include <SDL2/SDL.h>
namespace validation {
class PassScope {
    Pass* pass = nullptr;
    Uint64 beginCpu = 0;
    GLuint queries[2] = {0, 0};
public:
    explicit PassScope(const char* name) {
        if (!run.capture) return;
        trackrs::draw::flush();
        pass = &run.passes.at(name);
        beginCpu = SDL_GetPerformanceCounter();
        if ((GLAD_GL_VERSION_3_3 || GLAD_GL_ARB_timer_query)) {
            glGenQueries(2, queries);
            glQueryCounter(queries[0], GL_TIMESTAMP);
        }
    }
    ~PassScope() {
        if (!pass) return;
        trackrs::draw::flush();
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
