// SPDX-License-Identifier: GPL-2.0-or-later
#include "rhi.h"
#include <cstdio>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>
namespace trackrs::rhi {
namespace {
// Khronos callback ABI prescribes adjacent enum parameters.
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void GLAD_API_PTR debugCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei, const GLchar *message,
                                const void *user) {
  auto &debug =
      *const_cast<DebugMessages *>(static_cast<const DebugMessages *>(user));
  // Shader compiler diagnostics are recoverable during hot reload, never API
  // errors.
  if ((type == GL_DEBUG_TYPE_ERROR ||
       type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR ||
       severity == GL_DEBUG_SEVERITY_HIGH) &&
      source != GL_DEBUG_SOURCE_SHADER_COMPILER)
    ++debug.errors;
  if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
    return;
  std::fprintf(stderr, "GL debug %u [%x/%x]: %.1024s\n", id, source, type,
               message);
}
std::string read(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  const auto size = file.tellg();
  if (!file || size < 0 || size > std::streamoff{1024} * 1024)
    throw std::runtime_error("shader read failed: " + path.string());
  std::string text(static_cast<std::size_t>(size), '\0');
  file.seekg(0);
  file.read(text.data(), size);
  if (!file)
    throw std::runtime_error("truncated shader: " + path.string());
  return text;
}
using WatchMap =
    std::map<std::filesystem::path, std::filesystem::file_time_type>;
std::filesystem::file_time_type modified(const std::filesystem::path &path) {
  std::error_code error;
  const auto time = std::filesystem::last_write_time(path, error);
  return error ? std::filesystem::file_time_type::min() : time;
}
struct SourceLoader {
  std::filesystem::path root;
  WatchMap &watched;
  std::set<std::filesystem::path> active;
  std::size_t total = 0;
  std::string load(const std::filesystem::path &requested) {
    const auto path = std::filesystem::weakly_canonical(requested);
    const auto relative = path.lexically_relative(root);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..")
      throw std::runtime_error("shader include escapes source directory");
    watched[path] = modified(path);
    if (active.size() >= 16 || !active.insert(path).second)
      throw std::runtime_error("shader include cycle/depth exceeded");
    const auto text = read(path);
    total += text.size();
    if (total > std::size_t{1024} * 1024)
      throw std::runtime_error("expanded shader exceeds size limit");
    std::istringstream input(text);
    std::string line, result;
    unsigned number = 0;
    const std::regex include(
        R"regex(^\s*#include\s*"([^"]+)"\s*(?://.*)?$)regex");
    while (std::getline(input, line)) {
      ++number;
      std::smatch match;
      if (std::regex_match(line, match, include))
        result += "\n#line 1\n" + load(path.parent_path() / match[1].str()) +
                  "\n#line " + std::to_string(number + 1) + "\n";
      else
        result += line + '\n';
    }
    active.erase(path);
    return result;
  }
};
std::string permutation(std::string text,
                        const std::vector<std::string> &defines) {
  if (defines.empty())
    return text;
  const std::regex identifier(
      R"(^[A-Za-z_][A-Za-z_0-9]*( [A-Za-z_0-9.+-]+)?$)");
  std::string header;
  for (const auto &define : defines) {
    if (!std::regex_match(define, identifier))
      throw std::invalid_argument("invalid shader define");
    header += "#define " + define + '\n';
  }
  const auto version = text.find("#version");
  const auto end = text.find('\n', version);
  if (version == std::string::npos || end == std::string::npos)
    throw std::runtime_error("shader define requires version line");
  text.insert(end + 1, header);
  return text;
}
GLuint compile(GLenum stage, const std::string &text) {
  const GLuint shader = glCreateShader(stage);
  const char *data = text.c_str();
  glShaderSource(shader, 1, &data, nullptr);
  glCompileShader(shader);
  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char message[4096]{};
    glGetShaderInfoLog(shader, sizeof(message), nullptr, message);
    glDeleteShader(shader);
    throw std::runtime_error(message);
  }
  return shader;
}
GLuint link(const std::string &vertex, const std::string &fragment) {
  struct Shader {
    GLuint id = 0;
    ~Shader() {
      if (id)
        glDeleteShader(id);
    }
  } vs, fs;
  vs.id = compile(GL_VERTEX_SHADER, vertex);
  fs.id = compile(GL_FRAGMENT_SHADER, fragment);
  const GLuint program = glCreateProgram();
  glAttachShader(program, vs.id);
  glAttachShader(program, fs.id);
  glLinkProgram(program);
  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char message[4096]{};
    glGetProgramInfoLog(program, sizeof(message), nullptr, message);
    glDeleteProgram(program);
    throw std::runtime_error(message);
  }
  return program;
}
} // namespace
void DebugMessages::install() {
  if (!GLAD_GL_KHR_debug || !glDebugMessageCallback)
    throw std::runtime_error("KHR_debug required for the 2a gate");
  glEnable(GL_DEBUG_OUTPUT);
  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  glDebugMessageCallback(debugCallback, this);
}
void DebugMessages::check() const {
  if (errors.load())
    throw std::runtime_error("fatal core GL debug API error");
}
void StateCache::program(GLuint value) {
  if (program_ != value) {
    glUseProgram(value);
    program_ = value;
    ++changes_;
  }
}
void StateCache::vertexArray(GLuint value) {
  if (vao_ != value) {
    glBindVertexArray(value);
    vao_ = value;
    ++changes_;
  }
}
void StateCache::texture(GLuint value) {
  if (texture_ != value) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, value);
    texture_ = value;
    ++changes_;
  }
}
void StateCache::viewport(int width, int height) {
  const std::array<int, 4> value{{0, 0, width, height}};
  if (viewport_ != value) {
    glViewport(0, 0, width, height);
    viewport_ = value;
    ++changes_;
  }
}
void StateCache::presentationState() {
  if (initialized_)
    return;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  for (GLenum cap : {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST,
                     GL_FRAMEBUFFER_SRGB, GL_DITHER})
    glDisable(cap);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  initialized_ = true;
  ++changes_;
}
ShaderProgram::ShaderProgram(std::filesystem::path vertex,
                             std::filesystem::path fragment,
                             std::function<bool(GLuint)> validate,
                             std::vector<std::string> defines)
    : vertex_(std::move(vertex)), fragment_(std::move(fragment)),
      defines_(std::move(defines)), validate_(std::move(validate)) {
  if (!poll(true))
    throw std::runtime_error("initial shader link failed: " + error_);
}
ShaderProgram::~ShaderProgram() {
  if (id_)
    glDeleteProgram(id_);
}
bool ShaderProgram::poll(bool force) {
  const auto now = std::chrono::steady_clock::now();
  if (!force && now < nextPoll_)
    return false;
  nextPoll_ = now + std::chrono::milliseconds(250);
  try {
    bool changed = force || watched_.empty();
    for (const auto &entry : watched_)
      if (modified(entry.first) != entry.second)
        changed = true;
    if (!changed)
      return false;
    watched_.clear();
    watched_[std::filesystem::weakly_canonical(vertex_)] = modified(vertex_);
    watched_[std::filesystem::weakly_canonical(fragment_)] =
        modified(fragment_);
    auto sourceRoot = std::filesystem::weakly_canonical(vertex_.parent_path());
    // Built-in stages share bounded includes across shaders/basic,pbr,world.
    // Arbitrary tool/test directories retain the original single-folder
    // sandbox.
    if (sourceRoot.parent_path().filename() == "shaders")
      sourceRoot = sourceRoot.parent_path();
    auto fragmentRelative =
        std::filesystem::weakly_canonical(fragment_).lexically_relative(
            sourceRoot);
    if (fragmentRelative.empty() || fragmentRelative.is_absolute() ||
        *fragmentRelative.begin() == "..")
      throw std::runtime_error("shader stages require one source tree");
    SourceLoader loader{sourceRoot, watched_, {}, 0};
    const auto vertex = permutation(loader.load(vertex_), defines_);
    const auto fragment = permutation(loader.load(fragment_), defines_);
    struct FreshProgram {
      GLuint id;
      ~FreshProgram() {
        if (id)
          glDeleteProgram(id);
      }
    } fresh{link(vertex, fragment)};
    if (validate_ && !validate_(fresh.id))
      throw std::runtime_error("shader interface contract rejected");
    if (id_)
      glDeleteProgram(id_);
    id_ = std::exchange(fresh.id, 0);
    error_.clear();
    return true;
  } catch (const std::exception &error) {
    error_ = error.what();
    std::fprintf(stderr,
                 "Shader reload rejected; last good program retained: %s\n",
                 error_.c_str());
    return false;
  }
}
UniformBuffer::UniformBuffer(std::size_t bytes) : size_(bytes) {
  if (!bytes)
    throw std::invalid_argument("empty UBO");
  glGenBuffers(1, &id_);
  glBindBuffer(GL_UNIFORM_BUFFER, id_);
  glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(bytes), nullptr,
               GL_DYNAMIC_DRAW);
}
UniformBuffer::~UniformBuffer() {
  if (id_)
    glDeleteBuffers(1, &id_);
}
void UniformBuffer::update(const void *bytes, std::size_t size) {
  if (size != size_ || !bytes)
    throw std::invalid_argument("UBO size mismatch");
  glBindBuffer(GL_UNIFORM_BUFFER, id_);
  glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(size), bytes);
}
void UniformBuffer::bind(unsigned binding) const {
  glBindBufferBase(GL_UNIFORM_BUFFER, binding, id_);
}
} // namespace trackrs::rhi
