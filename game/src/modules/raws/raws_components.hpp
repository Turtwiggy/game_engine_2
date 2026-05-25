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

SpriteComponent
default_sprite();

struct SpawnConfig
{
  const vec2 pos;
  const vec2 size;
  const ColourComponent colour = { 1.0f, 1.0f, 1.0f, 1.0f };
  const SpriteComponent sprite = default_sprite();
  const bool is_static = false;
  const bool is_sensor = false;
  const bool is_emitter = false;
  const bool is_occluder = false;
};
entt::entity
spawn(entt::registry& r, const SpawnConfig& data);

} // namespace game2d