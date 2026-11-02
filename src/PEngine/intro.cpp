// SPDX-License-Identifier: GPL-2.0-or-later
/** In-window, skippable intro. All GL/audio handles die before their contexts.
 * Application thread only; optional media errors retain the licensed splash.
 */
#include "pengine.h"
#include "app.h"
#include "validation.h"
#include "../../engine/media/movie.h"
#include "../../engine/core/frame_pacer.h"
#include <memory>
void PApp::playIntro() {
    if (validation::run.options.enabled || !PHYSFS_exists("/intro.mpg")) return;
    try {
        std::unique_ptr<PHYSFS_File, decltype(&PHYSFS_close)> file(PHYSFS_openRead("/intro.mpg"), &PHYSFS_close);
        if (!file) return;
        const auto length = PHYSFS_fileLength(file.get());
        if (length <= 0 || length > 64 * 1024 * 1024) throw std::runtime_error("intro exceeds size limit");
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        if (PHYSFS_readBytes(file.get(), bytes.data(), length) != length) throw std::runtime_error("intro read failed");
        trackrs::Movie movie(std::move(bytes));
        struct Texture { GLuint id = 0; ~Texture() { if (id) glDeleteTextures(1, &id); } } texture;
        struct Audio { SDL_AudioDeviceID id = 0; ~Audio() { if (id) SDL_CloseAudioDevice(id); } } audio;
        if (movie.sampleRate() > 0 && SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
            SDL_AudioSpec wanted{}; wanted.freq = movie.sampleRate(); wanted.format = AUDIO_F32SYS;
            wanted.channels = 2; wanted.samples = 1024;
            audio.id = SDL_OpenAudioDevice(nullptr, 0, &wanted, nullptr, 0);
            if (audio.id) SDL_PauseAudioDevice(audio.id, 0);
        }
        glGenTextures(1, &texture.id); glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, movie.width(), movie.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_LIGHTING); glDisable(GL_FOG);
        glDisable(GL_BLEND); glEnable(GL_TEXTURE_2D); glColor4f(1,1,1,1);
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
        PUtil::outLog() << "In-window pl_mpeg intro started " << movie.width() << "x" << movie.height() << "\n";
        auto previous = SDL_GetPerformanceCounter();
        trackrs::FramePacer pacer;
        bool skip = false;
        while (!skip && !movie.ended()) {
            pacer.begin();
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) { requestExit(); skip = true; }
                if (event.type == SDL_KEYDOWN || event.type == SDL_MOUSEBUTTONDOWN) skip = true;
            }
            const auto now = SDL_GetPerformanceCounter();
            const auto elapsed = static_cast<double>(now - previous) / static_cast<double>(SDL_GetPerformanceFrequency());
            previous = now;
            if (movie.advance(elapsed)) {
                glBindTexture(GL_TEXTURE_2D, texture.id); glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, movie.width(), movie.height(), GL_RGB, GL_UNSIGNED_BYTE, movie.rgb().data());
            }
            if (audio.id && !movie.audio().empty() && SDL_GetQueuedAudioSize(audio.id) < 384000)
                SDL_QueueAudio(audio.id, movie.audio().data(), static_cast<Uint32>(movie.audio().size() * sizeof(float)));
            glClear(GL_COLOR_BUFFER_BIT); glBindTexture(GL_TEXTURE_2D, texture.id);
            const float aspect = static_cast<float>(movie.width()) / static_cast<float>(movie.height());
            const float view = static_cast<float>(cx) / static_cast<float>(cy);
            const float x = std::min(1.0f, aspect / view), y = std::min(1.0f, view / aspect);
            glBegin(GL_QUADS);
            glTexCoord2f(0,1); glVertex2f(-x,-y); glTexCoord2f(1,1); glVertex2f(x,-y);
            glTexCoord2f(1,0); glVertex2f(x,y); glTexCoord2f(0,0); glVertex2f(-x,y); glEnd();
            SDL_GL_SwapWindow(screen); pacer.wait(60);
        }
        glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopAttrib();
        PUtil::outLog() << "In-window pl_mpeg intro completed or skipped\n";
    } catch (const std::exception& error) {
        PUtil::outLog() << "Intro fallback: " << error.what() << '\n';
    }
}
