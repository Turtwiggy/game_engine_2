#include "pch.hpp" // IWYU pragma: keep

#include "event_shoot_log_helpers.hpp"

#include "game_and_engine_interop.hpp"
#include "modules/raws/raws_components.hpp"
#include "systems/system_shoot/shoot_components.hpp"

namespace game2d {

void
handle_shoot_event__log(entt::registry& r, const ShootEvent& evt)
{
  const auto wep_e = evt.weapon_e;

  if (wep_e == entt::null || !r.valid(wep_e))
    return;

  const auto& weapon_c = r.get<WeaponComponent>(wep_e);
  const auto& wep_t = r.get<const TransformComponent>(wep_e);
  const auto wep_pos = vec2{ wep_t.pos.x, wep_t.pos.y };

  // const auto wep_def = get_weapon_def(r, wep_e);
  // const auto bul_def = get_bullet_def(r, wep_e);
  WeaponDef altered_w_def = weapon_c.wep_def;
  BulletDef altered_b_def = weapon_c.bul_def;

  // using directly from transform is pretty sketch
  // const float shoot_angle = wep_t.rotation_radians.z;

  for (int i = 0; i < altered_w_def.projectiles; i++) {

    // const auto bullet_dir = engine::normalize_safe(engine::angle_radians_to_direction(ar[i]));
    // const auto bullet_vel = altered_b_def.speed * b2Vec2{ bullet_dir.x, bullet_dir.y };
    const auto bul_vel = vec2{ 0.0f, 0.0f };
    const auto bullet_e = spawn_projectile(r, wep_e, altered_b_def, wep_pos, bul_vel);
  }

  //
};

} // namespace game2d