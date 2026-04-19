#pragma once

#include "game_and_engine_interop.hpp"
#include <entt/fwd.hpp>

namespace game2d {

void
update_physics_system(GameData* data, entt::registry& r, const float dt);

} // namespace game2d