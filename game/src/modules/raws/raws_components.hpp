#pragma once

#include <entt/fwd.hpp>

#include "game_and_engine_interop.hpp"

namespace game2d {

// struct Raws
// {
//   bool placeholder = true;
// };

// Raws
// load_raws(std::string path);

// void
// give_life(entt::registry& r,
//           const entt::entity e,
//           const glm::vec2& pos,
//           const glm::vec2& size = { default_map_unit_tilesize, default_map_unit_tilesize });

entt::entity
spawn(entt::registry& r,
      const vec2 pos,
      const vec2 size,
      const ColourComponent colour,
      const bool is_static = false,
      const bool is_sensor = false,
      const bool is_emitter = false,
      const bool is_occlude = false);

} // namespace game2d