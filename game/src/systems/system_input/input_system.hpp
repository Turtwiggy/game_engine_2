#pragma once

#include "game_and_engine_interop.hpp"

#include "singleton.hpp"

#include <entt/fwd.hpp>

namespace game2d {

struct SINGLE_FrameInput : public Singleton<SINGLE_FrameInput>
{
  bool jump = false;
  bool pickup = false;
  bool request_gameover = false;
  vec2 keyboard_r = { 0.0f, 0.0f };
  vec2 keyboard_l = { 0.0f, 0.0f };
};

void
update_input_system(entt::registry& r, GameData* data);

} // namespace game2d