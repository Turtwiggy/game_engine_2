#pragma once

namespace game2d {

struct BoneInfo
{
  std::string name;
  int id = -1;
  glm::mat4 offset = glm::mat4(1.0);
};

constexpr int MAX_BONE_INFLUENCE = 4;
struct MeshVertex
{
  glm::ivec4 bone_ids;
  glm::vec4 weights;
  glm::vec4 position;
  glm::vec2 uv;

  // glm::vec4 colour = glm::vec4(232 / 255.0f, 97 / 255.0f, 160 / 255.0f, 1.0f);
  // glm::vec3 normal{ 0, 0, 0 };
};
struct MeshIndex
{
  Uint16 index;
};

struct Mesh
{
  std::string name;
  std::vector<MeshVertex> vertex_data;
  std::vector<MeshIndex> index_data;

  SDL_GPUBuffer* vertex_buffer;
  SDL_GPUBuffer* index_buffer;
};

struct Model
{
  std::string path;
  std::vector<Mesh> meshes;
  std::vector<BoneInfo> bone_info;
};

struct SINGLE_ModelsComponent
{
  Model model_0_data;
};

} // namespace game2d