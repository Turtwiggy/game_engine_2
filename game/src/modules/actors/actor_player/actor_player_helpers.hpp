#pragma once

#include "game_and_engine_interop.hpp"

#include <entt/fwd.hpp>

namespace game2d {

void
load_spritelayer_textures(entt::registry& r, GameData* data);

void
attach_spritelayers(entt::registry& r, entt::entity player_e, const GameData* data);

} // namespace game2d