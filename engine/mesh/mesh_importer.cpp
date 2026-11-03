// SPDX-License-Identifier: GPL-2.0-or-later
#include "mesh_importer.h"
#include <cgltf.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <memory>
#include <meshoptimizer.h>
#include <nlohmann/json.hpp>
#include <sstream>
namespace trackrs::mesh {
namespace {
struct Input {
  ReadFile reader;
  std::filesystem::path root;
  std::size_t remaining;
  Bytes read(const std::string &filename) {
    auto path = std::filesystem::path(filename).lexically_normal();
    auto rel = path.lexically_relative(root);
    if (rel.empty() || *rel.begin() == ".." ||
        filename.find("://") != std::string::npos)
      throw std::runtime_error("mesh URI escapes asset directory: " + filename);
    Bytes bytes;
    if (reader)
      bytes = reader(path.generic_string());
    else {
      std::ifstream file(path, std::ios::binary | std::ios::ate);
      if (!file)
        throw std::runtime_error("cannot read mesh dependency: " + filename);
      auto length = file.tellg();
      if (length < 0 || static_cast<std::uintmax_t>(length) > remaining)
        throw std::runtime_error("mesh byte budget exceeded");
      bytes.resize(static_cast<std::size_t>(length));
      file.seekg(0);
      if (!file.read(reinterpret_cast<char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("short mesh read");
    }
    if (bytes.size() > remaining)
      throw std::runtime_error("mesh byte budget exceeded");
    remaining -= bytes.size();
    return bytes;
  }
  Bytes uri(const char *value) {
    if (!value)
      return {};
    std::string uri(value);
    if (uri.rfind("data:", 0) == 0) {
      auto comma = uri.find(',');
      if (comma == std::string::npos ||
          uri.substr(0, comma).find(";base64") == std::string::npos)
        throw std::runtime_error("unsupported mesh data URI");
      Bytes result;
      unsigned bits = 0, count = 0;
      for (std::size_t i = comma + 1; i < uri.size(); ++i) {
        char c = uri[i];
        if (c == '=')
          break;
        const std::string alphabet =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        auto n = alphabet.find(c);
        if (n == std::string::npos)
          throw std::runtime_error("invalid base64 image");
        bits = (bits << 6) | static_cast<unsigned>(n);
        count += 6;
        if (count >= 8) {
          count -= 8;
          result.push_back(static_cast<std::uint8_t>(bits >> count));
          if (result.size() > remaining)
            throw std::runtime_error("mesh byte budget exceeded");
        }
      }
      remaining -= result.size();
      return result;
    }
    if (uri.find('%') != std::string::npos ||
        uri.find('\\') != std::string::npos ||
        std::filesystem::path(uri).is_absolute())
      throw std::runtime_error("unsupported mesh URI");
    return read((root / uri).generic_string());
  }
};
cgltf_result readBuffer(const cgltf_memory_options *,
                        const cgltf_file_options *options, const char *path,
                        cgltf_size *size, void **data) {
  try {
    auto bytes = static_cast<Input *>(options->user_data)->read(path);
    if (*size && bytes.size() < *size)
      return cgltf_result_data_too_short;
    void *allocation = std::malloc(bytes.size());
    if (!allocation)
      return cgltf_result_out_of_memory;
    std::memcpy(allocation, bytes.data(), bytes.size());
    *size = bytes.size();
    *data = allocation;
    return cgltf_result_success;
  } catch (...) {
    return cgltf_result_io_error;
  }
}
void releaseBuffer(const cgltf_memory_options *, const cgltf_file_options *,
                   void *data) {
  std::free(data);
}
Texture texture(Input &input, const cgltf_texture_view &view) {
  Texture out;
  if (!view.texture)
    return out;
  if (view.texcoord != 0 || view.has_transform)
    throw std::runtime_error(
        "only UV0 without texture transforms is supported");
  auto image = view.texture->image;
  if (!image)
    throw std::runtime_error("texture has no supported PNG/JPEG source");
  out.name = image->name ? image->name : image->uri ? image->uri : "embedded";
  // Keep portable glTF PNG/JPEG fallbacks. TrackRS native BC KTX2 caches
  // are selected through optional image extras rather than mislabelled BasisU.
  std::string native;
  if (image->extras.data) {
    auto extras = nlohmann::json::parse(image->extras.data);
    if (extras.contains("trackrsKtx2"))
      native = extras.at("trackrsKtx2").get<std::string>();
  }
  if (!native.empty()) {
    out.name = native;
    out.encoded = input.uri(native.c_str());
  } else if (image->uri)
    out.encoded = input.uri(image->uri);
  else if (image->buffer_view) {
    auto v = image->buffer_view;
    auto begin = cgltf_buffer_view_data(v);
    if (!begin || v->size > input.remaining)
      throw std::runtime_error("invalid embedded image");
    out.encoded.assign(begin, begin + v->size);
    input.remaining -= v->size;
  }
  return out;
}
void finish(Submesh &mesh, std::vector<std::uint32_t> colors,
            const ImportOptions &options, MeshData &output) {
  if (mesh.indices.size() > options.maxCorners - output.sourceCorners)
    throw std::runtime_error("mesh corner budget exceeded");
  output.sourceCorners += mesh.indices.size();
  computeTangents(mesh.vertices, mesh.indices, &colors);
  struct Colored {
    Vertex vertex;
    std::uint32_t color;
  };
  static_assert(sizeof(Colored) == 60);
  std::vector<Colored> input(mesh.vertices.size());
  for (std::size_t i = 0; i < input.size(); ++i)
    input[i] = {mesh.vertices[i], colors[i]};
  if (options.optimize && !input.empty()) {
    std::vector<unsigned> remap(input.size());
    auto count = meshopt_generateVertexRemap(remap.data(), mesh.indices.data(),
                                             mesh.indices.size(), input.data(),
                                             input.size(), sizeof(Colored));
    std::vector<Colored> compact(count);
    meshopt_remapVertexBuffer(compact.data(), input.data(), input.size(),
                              sizeof(Colored), remap.data());
    meshopt_remapIndexBuffer(mesh.indices.data(), mesh.indices.data(),
                             mesh.indices.size(), remap.data());
    meshopt_optimizeVertexCache(mesh.indices.data(), mesh.indices.data(),
                                mesh.indices.size(), count);
    meshopt_optimizeOverdraw(mesh.indices.data(), mesh.indices.data(),
                             mesh.indices.size(), compact[0].vertex.position,
                             count, sizeof(Colored), 1.05f);
    meshopt_optimizeVertexFetch(compact.data(), mesh.indices.data(),
                                mesh.indices.size(), compact.data(), count,
                                sizeof(Colored));
    input = std::move(compact);
  }
  mesh.vertices.resize(input.size());
  mesh.packed.resize(input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    mesh.vertices[i] = input[i].vertex;
    mesh.packed[i] = packVertex(input[i].vertex, input[i].color);
  }
  output.submeshes.push_back(std::move(mesh));
  output.optimized = options.optimize;
}
void generateNormals(Submesh &mesh) {
  for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
    auto &a = mesh.vertices[mesh.indices[i]];
    auto &b = mesh.vertices[mesh.indices[i + 1]];
    auto &c = mesh.vertices[mesh.indices[i + 2]];
    auto n =
        glm::cross(glm::make_vec3(b.position) - glm::make_vec3(a.position),
                   glm::make_vec3(c.position) - glm::make_vec3(a.position));
    if (glm::dot(n, n) < 1e-20f)
      throw std::runtime_error("degenerate mesh triangle");
    n = glm::normalize(n);
    for (auto j : {i, i + 1, i + 2})
      std::copy_n(glm::value_ptr(n), 3, mesh.vertices[mesh.indices[j]].normal);
  }
}
MeshData gltf(const std::string &filename, Input &input,
              const ImportOptions &settings) {
  auto bytes = input.read(filename);
  cgltf_options options{};
  options.file = {readBuffer, releaseBuffer, &input};
  cgltf_data *raw = nullptr;
  if (cgltf_parse(&options, bytes.data(), bytes.size(), &raw) !=
      cgltf_result_success)
    throw std::runtime_error("cgltf parse failed");
  std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, cgltf_free);
  std::size_t bufferBytes = 0;
  for (std::size_t i = 0; i < raw->buffers_count; ++i) {
    if (raw->buffers[i].size > input.remaining - bufferBytes)
      throw std::runtime_error("glTF buffer budget exceeded");
    bufferBytes += raw->buffers[i].size;
  }
  if (cgltf_load_buffers(&options, raw, filename.c_str()) !=
          cgltf_result_success ||
      cgltf_validate(raw) != cgltf_result_success)
    throw std::runtime_error("cgltf buffers/validation failed");
  MeshData out;
  for (std::size_t i = 0; i < raw->materials_count; ++i) {
    const auto &source = raw->materials[i];
    Material m;
    m.name = source.name ? source.name : "material";
    const auto &p = source.pbr_metallic_roughness;
    std::copy_n(p.base_color_factor, 4, m.baseColor.begin());
    m.metallic = p.metallic_factor;
    m.roughness = p.roughness_factor;
    m.normalScale = source.normal_texture.scale;
    m.alphaCutoff = source.alpha_cutoff;
    m.doubleSided = source.double_sided != 0;
    m.alpha = source.alpha_mode == cgltf_alpha_mode_mask    ? AlphaMode::Mask
              : source.alpha_mode == cgltf_alpha_mode_blend ? AlphaMode::Blend
                                                            : AlphaMode::Opaque;
    m.albedo = texture(input, p.base_color_texture);
    m.normal = texture(input, source.normal_texture);
    m.metalRough = texture(input, p.metallic_roughness_texture);
    if (source.occlusion_texture.texture ==
            p.metallic_roughness_texture.texture &&
        source.occlusion_texture.texture)
      m.aoStrength = source.occlusion_texture.scale;
    if (source.has_clearcoat) {
      m.clearcoat = source.clearcoat.clearcoat_factor;
      m.clearcoatRoughness = source.clearcoat.clearcoat_roughness_factor;
    }
    if (!source.unlit && !m.albedo.empty() && !m.normal.empty() &&
        !m.metalRough.empty())
      m.path = MaterialPath::Pbr;
    out.materials.push_back(std::move(m));
  }
  out.materials.push_back(Material{});
  auto visit = [&](auto &&self, const cgltf_node *node) -> void {
    if (node->skin || node->weights_count)
      throw std::runtime_error("skinned/morph meshes are unsupported");
    if (node->mesh) {
      float matrix[16];
      cgltf_node_transform_world(node, matrix);
      glm::mat4 model = glm::make_mat4(matrix);
      if (settings.gltfYUpToZUp)
        model =
            glm::rotate(glm::mat4(1), glm::radians(90.f), glm::vec3(1, 0, 0)) *
            model;
      const float det = glm::determinant(glm::mat3(model));
      if (!std::isfinite(det) || std::abs(det) < 1e-12f)
        throw std::runtime_error("singular mesh node");
      auto normal = glm::transpose(glm::inverse(glm::mat3(model)));
      for (std::size_t p = 0; p < node->mesh->primitives_count; ++p) {
        const auto &primitive = node->mesh->primitives[p];
        if (primitive.type != cgltf_primitive_type_triangles ||
            primitive.targets_count || primitive.has_draco_mesh_compression)
          throw std::runtime_error(
              "only static triangle primitives are supported");
        const cgltf_accessor *positions = nullptr, *normals = nullptr,
                             *uvs = nullptr, *colors = nullptr;
        for (std::size_t a = 0; a < primitive.attributes_count; ++a) {
          auto attr = primitive.attributes[a];
          if (attr.index != 0)
            continue;
          if (attr.type == cgltf_attribute_type_position)
            positions = attr.data;
          if (attr.type == cgltf_attribute_type_normal)
            normals = attr.data;
          if (attr.type == cgltf_attribute_type_texcoord)
            uvs = attr.data;
          if (attr.type == cgltf_attribute_type_color)
            colors = attr.data;
        }
        if (!positions)
          throw std::runtime_error("mesh has no POSITION");
        auto count =
            primitive.indices ? primitive.indices->count : positions->count;
        if (count % 3 || count > settings.maxCorners - out.sourceCorners)
          throw std::runtime_error("invalid triangle count");
        Submesh mesh;
        mesh.name = node->name ? node->name : "node";
        mesh.material =
            primitive.material
                ? static_cast<std::size_t>(primitive.material - raw->materials)
                : out.materials.size() - 1;
        std::vector<std::uint32_t> tint;
        for (std::size_t corner = 0; corner < count; ++corner) {
          auto index =
              primitive.indices
                  ? cgltf_accessor_read_index(primitive.indices, corner)
                  : corner;
          if (index >= positions->count)
            throw std::runtime_error("mesh index out of bounds");
          Vertex v;
          float pos[3];
          if (!cgltf_accessor_read_float(positions, index, pos, 3))
            throw std::runtime_error("unsupported position accessor");
          auto wp = model * glm::vec4(glm::make_vec3(pos), 1);
          std::copy_n(glm::value_ptr(wp), 3, v.position);
          if (normals) {
            float n[3];
            if (index >= normals->count ||
                !cgltf_accessor_read_float(normals, index, n, 3))
              throw std::runtime_error("invalid normal accessor");
            auto wn = normal * glm::make_vec3(n);
            std::copy_n(glm::value_ptr(wn), 3, v.normal);
          }
          if (uvs && (index >= uvs->count ||
                      !cgltf_accessor_read_float(uvs, index, v.uv, 2)))
            throw std::runtime_error("invalid UV accessor");
          for (float value : v.position)
            if (!std::isfinite(value))
              throw std::runtime_error("non-finite position");
          for (float value : v.uv)
            if (!std::isfinite(value))
              throw std::runtime_error("non-finite UV");
          float color[4] = {1, 1, 1, 1};
          if (colors && (index >= colors->count ||
                         !cgltf_accessor_read_float(colors, index, color, 4)))
            throw std::runtime_error("invalid color accessor");
          mesh.vertices.push_back(v);
          mesh.indices.push_back(static_cast<std::uint32_t>(corner));
          tint.push_back(packRGBA8(color[0], color[1], color[2], color[3]));
        }
        if (det < 0)
          for (std::size_t i = 0; i < count; i += 3)
            std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        if (!normals)
          generateNormals(mesh);
        finish(mesh, std::move(tint), settings, out);
      }
    }
    for (std::size_t i = 0; i < node->children_count; ++i)
      self(self, node->children[i]);
  };
  if (raw->scene) {
    for (std::size_t n = 0; n < raw->scene->nodes_count; ++n)
      visit(visit, raw->scene->nodes[n]);
  } else
    for (std::size_t n = 0; n < raw->nodes_count; ++n)
      if (!raw->nodes[n].parent)
        visit(visit, &raw->nodes[n]);
  if (out.submeshes.empty())
    throw std::runtime_error("mesh scene is empty");
  return out;
}
MeshData obj(const std::string &filename, Input &input,
             const ImportOptions &settings) {
  auto bytes = input.read(filename);
  std::istringstream lines(std::string(bytes.begin(), bytes.end()));
  std::vector<glm::vec3> pos, norm;
  std::vector<glm::vec2> uv;
  MeshData out;
  out.materials.push_back(Material{});
  std::map<std::string, std::size_t> materials;
  std::map<std::size_t, Submesh> groups;
  std::size_t material = 0;
  std::string line;
  auto index = [](const std::string &token, std::size_t count) {
    auto value = std::stoll(token);
    auto resolved =
        value > 0 ? value - 1 : static_cast<long long>(count) + value;
    if (value == 0 || resolved < 0 ||
        static_cast<std::size_t>(resolved) >= count)
      throw std::runtime_error("OBJ index out of bounds");
    return static_cast<std::size_t>(resolved);
  };
  while (std::getline(lines, line)) {
    std::istringstream fields(line);
    std::string key;
    fields >> key;
    if (key == "v" || key == "vn") {
      glm::vec3 v;
      if (!(fields >> v.x >> v.y >> v.z) || !std::isfinite(glm::dot(v, v)))
        throw std::runtime_error("invalid OBJ vector");
      (key == "v" ? pos : norm).push_back(v);
    } else if (key == "vt") {
      glm::vec2 v;
      if (!(fields >> v.x >> v.y) || !std::isfinite(glm::dot(v, v)))
        throw std::runtime_error("invalid OBJ UV");
      uv.push_back(v);
    } else if (key == "usemtl") {
      std::string name;
      fields >> name;
      auto found = materials.find(name);
      if (found == materials.end()) {
        material = out.materials.size();
        materials[name] = material;
        Material m;
        m.name = name;
        out.materials.push_back(m);
      } else
        material = found->second;
    } else if (key == "mtllib") {
      std::string file;
      fields >> file;
      auto mbytes = input.uri(file.c_str());
      const auto materialDirectory = std::filesystem::path(file).parent_path();
      std::istringstream mlines(std::string(mbytes.begin(), mbytes.end()));
      Material *current = nullptr;
      while (std::getline(mlines, line)) {
        std::istringstream mf(line);
        mf >> key;
        if (key == "newmtl") {
          std::string name;
          mf >> name;
          auto it = materials.find(name);
          if (it == materials.end()) {
            materials[name] = out.materials.size();
            out.materials.push_back(Material{});
          }
          current = &out.materials[materials[name]];
          current->name = name;
        } else if (current && key == "Kd")
          mf >> current->baseColor[0] >> current->baseColor[1] >>
              current->baseColor[2];
        else if (current && key == "map_Kd") {
          mf >> file;
          const auto image = (materialDirectory / file).generic_string();
          current->albedo = {image, input.uri(image.c_str())};
        }
      }
    } else if (key == "f") {
      std::vector<Vertex> polygon;
      std::string token;
      bool missingNormal = false;
      while (fields >> token) {
        if (token[0] == '#')
          break;
        std::vector<std::string> parts;
        std::istringstream split(token);
        std::string part;
        while (std::getline(split, part, '/'))
          parts.push_back(part);
        Vertex v;
        auto p = pos.at(index(parts.at(0), pos.size()));
        std::copy_n(glm::value_ptr(p), 3, v.position);
        if (parts.size() > 1 && !parts[1].empty()) {
          auto t = uv.at(index(parts[1], uv.size()));
          std::copy_n(glm::value_ptr(t), 2, v.uv);
        }
        if (parts.size() > 2 && !parts[2].empty()) {
          auto n = norm.at(index(parts[2], norm.size()));
          std::copy_n(glm::value_ptr(n), 3, v.normal);
        } else
          missingNormal = true;
        polygon.push_back(v);
        if (polygon.size() > settings.maxCorners)
          throw std::runtime_error("OBJ face budget exceeded");
      }
      if (polygon.size() < 3)
        throw std::runtime_error("invalid OBJ polygon");
      auto &mesh = groups[material];
      mesh.material = material;
      mesh.name = out.materials[material].name;
      for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
        if (mesh.indices.size() + 3 > settings.maxCorners)
          throw std::runtime_error("OBJ corner budget exceeded");
        auto start = mesh.vertices.size();
        for (auto p : {std::size_t(0), i, i + 1}) {
          mesh.vertices.push_back(polygon[p]);
          mesh.indices.push_back(
              static_cast<std::uint32_t>(mesh.vertices.size() - 1));
        }
        if (missingNormal) {
          auto n =
              glm::cross(glm::make_vec3(mesh.vertices[start + 1].position) -
                             glm::make_vec3(mesh.vertices[start].position),
                         glm::make_vec3(mesh.vertices[start + 2].position) -
                             glm::make_vec3(mesh.vertices[start].position));
          if (glm::dot(n, n) < 1e-20f)
            throw std::runtime_error("degenerate OBJ triangle");
          n = glm::normalize(n);
          for (std::size_t v = start; v < start + 3; ++v)
            std::copy_n(glm::value_ptr(n), 3, mesh.vertices[v].normal);
        }
      }
    }
  }
  for (auto &pair : groups) {
    auto &mesh = pair.second;
    finish(mesh, std::vector<std::uint32_t>(mesh.vertices.size(), 0xffffffffu),
           settings, out);
  }
  if (out.submeshes.empty())
    throw std::runtime_error("OBJ has no faces");
  return out;
}
} // namespace
MeshData MeshImporter::load(const std::string &filename, const ReadFile &reader,
                            const ImportOptions &options) {
  auto path = std::filesystem::path(filename).lexically_normal();
  auto root = path.parent_path();
  if (root.empty())
    root = ".";
  Input input{reader, root, options.maxBytes};
  auto ext = path.extension().string();
  if (ext == ".gltf" || ext == ".glb")
    return gltf(path.generic_string(), input, options);
  if (ext == ".obj")
    return obj(path.generic_string(), input, options);
  throw std::runtime_error("unsupported mesh format: " + ext);
}
} // namespace trackrs::mesh
