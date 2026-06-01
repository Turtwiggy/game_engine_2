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

const int base_spritesheet_idx = 0;
const int char_spritesheet_idx = 1;

SpriteComponent
default_spritesheet();

struct PhysicsBodyDef
{
  bool is_bullet = false;
  bool is_sensor = false;
  bool is_static = false;
  float linear_damping = 0.0f;
  float angular_damping = 0.0f;
};

struct SpawnConfig
{
  const vec2 pos;
  const vec2 render_size;
  const vec2 coll_size;
  const PhysicsBodyDef body_def;
  const ColourComponent colour = { 1.0f, 1.0f, 1.0f, 1.0f };
  const SpriteComponent sprite = default_spritesheet();
  const bool is_emitter = false;
  const bool is_occluder = false;
};
entt::entity
spawn(entt::registry& r, const SpawnConfig& data);

} // namespace game2d