#pragma once

#include <SDL3/SDL_events.h>

#include <vector>

namespace game2d {

// Note: GameData shouldnt be here
// should be in some sort of shared file with the engine.
struct GameData
{
  Uint64 counter = 0;
  float dt = 0.0f;
  std::vector<SDL_Event> evts;
};

// Stop mangling the function names
extern "C"
{
  __declspec(dllexport) void game_init(GameData* data);
  __declspec(dllexport) void game_update();
  __declspec(dllexport) void game_refresh(GameData* data);
}

} // namespace game2d