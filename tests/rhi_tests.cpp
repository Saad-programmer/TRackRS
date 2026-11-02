// SPDX-License-Identifier: GPL-2.0-or-later
/** Real core-context integration probe. Owns temporary shader copies and GL
 * names; single render thread; no game assets or compatibility context is
 * needed here.
 */
#include "engine/rhi/rhi.h"
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace trackrs::rhi;
static void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void coreRenderTests(const std::filesystem::path &shaders);
int main(int argc, char **argv) {
  if (argc != 3)
    return 2;
  SDL_Window *window = nullptr;
  SDL_GLContext context = nullptr;
  std::filesystem::path temporary;
  int result = 0;
  try {
    SDL_SetMainReady();
    require(SDL_Init(SDL_INIT_VIDEO) == 0, SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    window = SDL_CreateWindow("RHI core integration", 0, 0, 64, 64,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    require(window != nullptr, SDL_GetError());
    context = SDL_GL_CreateContext(window);
    require(context != nullptr, SDL_GetError());
    require(gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)) !=
                0,
            "glad load failed");
    GLint mask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &mask);
    require((mask & GL_CONTEXT_CORE_PROFILE_BIT) != 0 && GLAD_GL_VERSION_3_3,
            "not GL 3.3 core");
    DebugMessages debug;
    debug.install();
    temporary =
        std::filesystem::temp_directory_path() /
        std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directory(temporary);
    const auto vertex = temporary / "test.vert",
               fragment = temporary / "test.frag";
    std::filesystem::copy_file(std::filesystem::path(argv[1]) / "present.vert",
                               vertex);
    std::filesystem::copy_file(std::filesystem::path(argv[1]) / "present.frag",
                               fragment);
    {
      ShaderProgram program(vertex, fragment,
                            [](GLuint candidate) {
                              return glGetUniformBlockIndex(candidate,
                                                            "Frame") !=
                                     GL_INVALID_INDEX;
                            },
                            {"RHI_GATE_DEFINE 1"});
      UniformBuffer buffer(16);
      const std::array<GLint, 4> extent{{0, 0, 2, 2}};
      buffer.update(extent.data(), sizeof(extent));
      buffer.bind(0);
      std::array<GLint, 4> read{};
      glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(read), read.data());
      require(read == extent, "UBO roundtrip failed");
      bool wrongSize = false;
      try {
        buffer.update(extent.data(), 4);
      } catch (const std::invalid_argument &) {
        wrongSize = true;
      }
      require(wrongSize, "UBO size contract not enforced");
      GLuint vao = 0, texture = 0;
      glGenVertexArrays(1, &vao);
      glGenTextures(1, &texture);
      StateCache cache;
      cache.presentationState();
      cache.viewport(2, 2);
      cache.program(program.id());
      cache.vertexArray(vao);
      cache.texture(texture);
      const auto calls = cache.changes();
      cache.presentationState();
      cache.viewport(2, 2);
      cache.program(program.id());
      cache.vertexArray(vao);
      cache.texture(texture);
      require(cache.changes() == calls, "redundant state changes issued");
      const std::array<unsigned char, 16> pixels{
          {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255}};
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA,
                   GL_UNSIGNED_BYTE, pixels.data());
      auto draw = [&] {
        cache.program(program.id());
        glUniformBlockBinding(program.id(),
                              glGetUniformBlockIndex(program.id(), "Frame"), 0);
        glUniform1i(glGetUniformLocation(program.id(), "legacyColor"), 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        std::array<unsigned char, 16> actual{};
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, actual.data());
        return actual;
      };
      require(draw() == pixels, "core texture/UBO rendering changed pixels");
      const auto original = program.id();
      std::ofstream(fragment) << "#version 330 core\ninvalid shader source\n";
      require(!program.poll(true) && program.id() == original &&
                  !program.error().empty(),
              "failed hot reload discarded good program");
      require(draw() == pixels, "failed reload changed rendering");
      std::ofstream(fragment) << "#version 330 core\nout vec4 outColor;void "
                                 "main(){outColor=vec4(1);}";
      require(!program.poll(true) && program.id() == original,
              "interface-invalid shader published");
      std::filesystem::copy_file(
          std::filesystem::path(argv[1]) / "present.frag", fragment,
          std::filesystem::copy_options::overwrite_existing);
      SDL_Delay(300);
      require(program.poll() && program.id() != original,
              "file-change hot reload failed");
      require(draw() == pixels, "successful reload changed identity output");
      const auto include = temporary / "probe.glsl";
      std::ofstream(include)
          << "#if RHI_GATE_DEFINE != 1\n#error missing "
             "permutation\n#endif\nvec4 shade(vec4 c){return c;}\n";
      std::ofstream(fragment)
          << "#version 330 core\n#include \"probe.glsl\"\nuniform sampler2D "
             "legacyColor;layout(std140) uniform Frame{ivec4 extent;};out vec4 "
             "outColor;void "
             "main(){outColor=shade(texelFetch(legacyColor,clamp(ivec2(gl_"
             "FragCoord.xy),ivec2(0),extent.zw-ivec2(1)),0));}\n";
      require(program.poll(true) && draw() == pixels,
              "include/permutation shader failed");
      std::ofstream(include) << "vec4 shade(vec4 c){return c.bgra;}\n";
      SDL_Delay(300);
      require(program.poll(), "included-file edit did not hot reload");
      auto swapped = pixels;
      for (std::size_t i = 0; i < swapped.size(); i += 4)
        std::swap(swapped[i], swapped[i + 2]);
      require(draw() == swapped, "included shader change not visible");
      const auto good = program.id();
      std::ofstream(include) << "#include \"probe.glsl\"\n";
      require(!program.poll(true) && program.id() == good,
              "include cycle accepted");
      std::ofstream(fragment)
          << "#version 330 core\n#include \"../outside.glsl\"\n";
      require(!program.poll(true) && program.id() == good,
              "out-of-root include accepted");
      debug.check();
      glDeleteTextures(1, &texture);
      glDeleteVertexArrays(1, &vao);
    }
    coreRenderTests(std::filesystem::path(argv[2]));
    debug.check();
    std::cerr << "EXPECTED_KHR_DEBUG_API_ERROR canary follows\n";
    glEnable(0xdead);
    require(debug.errors.load() > 0,
            "KHR_debug did not report real invalid API call");
    bool fatal = false;
    try {
      debug.check();
    } catch (const std::runtime_error &) {
      fatal = true;
    }
    require(fatal, "debug API error was not fatal");
    while (glGetError() != GL_NO_ERROR) {
    }
    debug.errors = 0;
    glDebugMessageCallback(nullptr, nullptr);
    std::cout << "RHI_GATE_OK: core profile, glad, identity pixels, cache, "
                 "UBO, hot reload rollback/recovery, core "
                 "UI/mesh/fog/light/SDF/instance-ring, fatal KHR_debug"
              << std::endl;
  } catch (const std::exception &error) {
    std::cerr << "RHI_GATE_FAILED: " << error.what() << '\n';
    result = 1;
  }
  if (context && glDebugMessageCallback)
    glDebugMessageCallback(nullptr, nullptr);
  if (context)
    SDL_GL_DeleteContext(context);
  if (window)
    SDL_DestroyWindow(window);
  SDL_Quit();
  if (!temporary.empty()) {
    std::error_code error;
    std::filesystem::remove_all(temporary, error);
  }
  return result;
}
