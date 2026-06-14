#include "pch.hpp" // IWYU pragma: keep

#include "shoot_system.hpp"

#include "game_and_engine_interop.hpp"
#include "modules/events/events_core/events_components.hpp"
#include "shoot_components.hpp"
#include "systems/system_input/input_components.hpp"

namespace game2d {

void
update_shoot_system(entt::registry& r, float dt)
{
  // #if defined(_DEBUG)
  //   ZoneScoped;
  // #endif

  auto view = r.view<WeaponComponent, TransformComponent, WeaponReloadRate, WeaponMagazine, WeaponFireRate>();
  for (const auto& [e, weapon_c, t_c, weapon_reload_c, weapon_mag_c, weapon_firerate_c] : view.each()) {

    const auto parent_e = weapon_c.parent_e;
    if (parent_e == entt::null || !r.valid(parent_e))
      continue;

    const auto& input_c = r.get<InputComponent>(parent_e);
    const auto& parent_pos_c = r.get<TransformComponent>(parent_e);

    // Set weapon position.
    auto dir = vec2{ input_c.rx, input_c.ry };
    t_c.pos = parent_pos_c.pos;
    t_c.pos += (0.5f * parent_pos_c.size);
    t_c.pos -= (0.5f * t_c.size);

    // Offset by dir
    t_c.pos.x += 16.0f * dir.x;
    t_c.pos.y += 16.0f * dir.y;

    // check your target is still within distance
    // (optional) theres a line of sight between you and it
    // const auto d = wep_pos - get_position(r, autofire_c.target);
    // const auto d2 = d.x * d.x + d.y * d.y;
    // const auto d2_threshold = pow(meters_to_pixels(wep_range_c.meters), 2);
    // if (d2 > d2_threshold)
    //   autofire_c.target = entt::null;
    // if (autofire_c.target == entt::null) {
    //   if (par_vel_m != b2Vec2_zero)
    //     aim_in_movement_direction(r, par_vel_m, wep_e);
    //   continue;
    // }

    auto aim_dir = dir;

    // you gotta reload
    if (weapon_reload_c.seconds_cur > 0.0) {
      weapon_reload_c.seconds_cur -= dt;
      continue;
    }

    // you've reloaded
    if (weapon_mag_c.bullets_cur <= 0) {
      weapon_mag_c.bullets_cur = weapon_c.wep_def.bullets_max;
      // weapon_fire_rate_c.seconds_between_shots_left = 0.0f;
    }

    // Check if you're fire-rate limited.
    weapon_firerate_c.seconds_between_shots_max = 1.0f / weapon_c.wep_def.firerate;
    if (weapon_firerate_c.seconds_between_shots_left >= 0.0f) {
      weapon_firerate_c.seconds_between_shots_left -= dt;
      continue;
    }

    // Check the clip size before firing
    if (weapon_mag_c.bullets_cur <= 0) { // time to reload
      weapon_reload_c.seconds_cur = weapon_c.wep_def.reloadrate;
      continue;
    }

    if (!input_c.shoot_down)
      continue;

    // Shoot a bullet!
    weapon_mag_c.bullets_cur--;
    if (weapon_mag_c.bullets_cur <= 0)
      weapon_reload_c.seconds_cur = weapon_c.wep_def.reloadrate;
    weapon_firerate_c.seconds_between_shots_left = weapon_firerate_c.seconds_between_shots_max;

    // do the shoot event
    ShootEvent shoot_evt;
    shoot_evt.weapon_e = e;
    shoot_evt.dir = aim_dir;
    auto& evts_c = SINGLE_Events::get();
    evts_c.dispatcher.trigger(shoot_evt);
    evts_c.dispatcher.update();
  }
}

} // namespace game2d