#pragma once

#include "common.hpp"

namespace game2d {

// Stop mangling the function names
extern "C"
{
  __declspec(dllexport) void game_init(GameData* data);
  __declspec(dllexport) void game_update();
  __declspec(dllexport) void game_refresh(GameData* data);
}

} // namespace game2d