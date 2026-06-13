#pragma once

#include <entt/fwd.hpp>

#include "systems/system_shoot/shoot_components.hpp"

namespace game2d {

void
handle_shoot_event__spawn_bullet(entt::registry& r, const ShootEvent& evt);

} // namespace game2d