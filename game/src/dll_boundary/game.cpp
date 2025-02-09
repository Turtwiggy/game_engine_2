#include "game.hpp"

#include <SDL3/SDL_log.h>
#include <format>

namespace game2d {

GameData* your_ref_to_data;

void
game_init(GameData* data)
{
  SDL_Log("(GAMEENGINE) Init()");
  your_ref_to_data = data;
  if (your_ref_to_data == NULL)
    return;
  GameData& data_ref = *your_ref_to_data;

  const auto str = std::format("(GameEngine-init) counter {}", data_ref.counter);
  SDL_Log("%s", str.c_str());
};

void
game_update()
{
  if (!your_ref_to_data)
    return;
  GameData& data_ref = *your_ref_to_data;

  data_ref.counter++;

  const auto str = std::format("(GameEngine-update) counter {}", data_ref.counter);
  SDL_Log("%s", str.c_str());
};

void
game_refresh(GameData* data)
{
  your_ref_to_data = data;
  if (!your_ref_to_data)
    return;
  GameData& data_ref = *your_ref_to_data;

  const auto str = std::format("(GameEngine-refresh) counter {}", data_ref.counter);
  SDL_Log("%s", str.c_str());
}

} // namespace game2d