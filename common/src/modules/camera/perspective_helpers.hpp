#pragma once

#include "game_and_engine_interop.hpp"
#include "perspective_components.hpp"

#include <entt/fwd.hpp>

namespace game2d {

glm::mat4
calculate_perspective_projection(int w, int h);

glm::mat4
calculate_perspective_view(const TransformComponent& t, const PerspectiveCamera& camera);

glm::quat
get_orientation(const PerspectiveCamera& camera);

glm::vec3
get_up_dir(const PerspectiveCamera& c);

glm::vec3
get_right_dir(const PerspectiveCamera& c);

glm::vec3
get_forward_dir(const PerspectiveCamera& c);

} // namespace game2d