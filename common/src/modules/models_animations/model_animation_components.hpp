#pragma once

#include "modules/models/model_components.hpp"

#include <entt/fwd.hpp>
#include <glm/gtx/quaternion.hpp>

namespace game2d {

struct KeyPosition
{
  glm::vec3 position;
  float timestamp = 0.0f;
};

struct KeyRotation
{
  glm::quat orientation;
  float timestamp = 0.0f;
};

struct KeyScale
{
  glm::vec3 scale;
  float timestamp = 0.0f;
};

struct Bone
{
  std::vector<KeyPosition> positions;
  std::vector<KeyRotation> rotations;
  std::vector<KeyScale> scales;

  glm::mat4 local_transform = glm::mat4(1.0f);
  std::string name;
  int id = 0;
};

struct AssimpNodeData
{
  glm::mat4 transformation;
  std::string name;
  std::vector<AssimpNodeData> children;
};

struct Animation
{
  float duration = 0.0f;
  float ticks = 0.0f;
  AssimpNodeData root;
  std::vector<Bone> bones;
  std::vector<BoneInfo> bone_info;
};

struct SINGLE_AnimatorComponent
{
  // result of loading, shouldnt be here
  Animation animation_0_data;

  // should be on a per-model basis?
  int amount_of_bones = 100;
  float current_time = 0.0f;
  Animation* current_animation;
  std::vector<glm::mat4> final_bone_matrices;
};

} // namespace game2d