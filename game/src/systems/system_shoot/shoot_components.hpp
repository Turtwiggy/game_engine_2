#pragma once

#include "modules/maths/vec.hpp"
#include <entt/fwd.hpp>

namespace game2d {

struct BulletDef
{
  entt::entity weapon_e;
  vec2 size{ 16.0f, 16.0f };
  int damage = 1;
  // int pierce = 0;
  // float speed = 1.0f;
  // int lifecycle = 3 * 1000;
  // float knockback_force = 1.0f;
  // int bounced = 0;
  // float crit_chance = 0.0f; // percent
  // float crit_damage = 100.0f;
  // float lifesteal = 0.0f;
};

struct WeaponDef
{
  int projectiles = 1;
  // int spread_deg = 30;
  float firerate = 0.5f;
  float reloadrate = 0.5f;
  // float range = 0.0f;
  int bullets_max = 5;
};

struct WeaponComponent
{
  entt::entity parent_e;
  const WeaponDef wep_def;
  const BulletDef bul_def;
};

struct BulletComponent
{
  entt::entity weapon_e;
};

//
//
//

struct WeaponFireRate
{
  float base_firerate = 2; // shots per second

  float seconds_between_shots_max = 1 / base_firerate; // 1.0/firerate
  float seconds_between_shots_left = 0.0f;             // the cooldown
};

struct WeaponReloadRate
{
  const float seconds_base_max = 0.5f;
  float seconds_cur = 0.0f;
};

// How many bullets to fire every time a bullet is fired?
struct WeaponProjectiles
{
  int projectiles = 1;

  // Given you're firing 1+ bullets,
  // and you're firing in a given direction,
  // what angle to add between the bullets?
  float angle_between_bullets_deg = 30;
};

// Distance at which the weapon can fire.
struct WeaponRange
{
  float meters = 3.0f;
};

struct WeaponMagazine
{
  int bullets_max = 1;
  int bullets_cur = 0;
};

struct ShootEvent
{
  entt::entity weapon_e;
};

} // namespace game2d