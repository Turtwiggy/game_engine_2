#pragma once

#include "game_and_engine_interop.hpp"

#include <entt/fwd.hpp>

namespace game2d {

void
update_input_system(entt::registry& r, const GameData* data);

} // namespace game2d