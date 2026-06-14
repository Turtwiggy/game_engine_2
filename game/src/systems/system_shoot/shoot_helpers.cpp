#include "pch.hpp" // IWYU pragma: keep

#include "systems/system_shoot/shoot_components.hpp"

#include "shoot_helpers.hpp"

namespace game2d {

void
set_weapon_components_from_weapon_def(entt::registry& r, entt::entity wep_e)
{
  const auto& weapon_c = r.get<WeaponComponent>(wep_e);
  const auto& wep_def = weapon_c.wep_def;

  auto& fr_c = r.get<WeaponFireRate>(wep_e);
  auto& rr_c = r.get<WeaponReloadRate>(wep_e);
  auto& wp_c = r.get<WeaponProjectiles>(wep_e);
  auto& wm_c = r.get<WeaponMagazine>(wep_e);

  fr_c.seconds_between_shots_max = wep_def.firerate;
  rr_c.seconds_base_max = wep_def.reloadrate;
  wp_c.projectiles = wep_def.projectiles;
  wm_c.bullets_cur = wep_def.bullets_max;
  wm_c.bullets_max = wep_def.bullets_max;
}

} // namespace game2d