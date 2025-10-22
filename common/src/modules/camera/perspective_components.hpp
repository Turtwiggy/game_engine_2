#pragma once

#include <entt/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace game2d {

struct PerspectiveCamera
{
  glm::mat4 view = glm::mat4(1.0f);
  glm::mat4 projection = glm::mat4(1.0f);
  float pitch = 0.0f;
  float yaw = 0.0f;
  float speed = 2.5f;
};

} // namespace game2d