#pragma once

#include <entt/fwd.hpp>

#include "game_and_engine_interop.hpp"
#include "systems/system_shoot/shoot_components.hpp"

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

SpriteComponent
default_spritesheet(int sprite_x = 0, int sprite_y = 0);

struct PhysicsBodyDef
{
  b2Vec2 pos_meters;
  b2Vec2 size_meters;
  bool is_bullet = false;
  bool is_sensor = false;
  bool is_static = false;
  float linear_damping = 0.0f;
  float angular_damping = 0.0f;
};
entt::entity
attach_body(entt::registry& r, entt::entity e, const PhysicsBodyDef& def);

struct SpriteDef
{
  const vec2 pos;
  const vec2 size;
  const ColourComponent colour = { 1.0f, 1.0f, 1.0f, 1.0f };
  const SpriteComponent sprite = default_spritesheet();
  const bool is_emitter = false;
  const bool is_occluder = false;
};
void
attach_sprite(entt::registry& r, entt::entity e, const SpriteDef& def);

//
//
//

entt::entity
spawn_projectile(entt::registry& r, const entt::entity weapon_e, const BulletDef& def, const vec2& pos, const vec2& vel);

} // namespace game2d