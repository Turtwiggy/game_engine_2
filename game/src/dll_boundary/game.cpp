#include "game.hpp"

#include <SDL3/SDL_log.h>

#include <format>

namespace game2d {

GameData* your_ref_to_data;

void
game_init(GameData* data)
{
  SDL_Log("(GameEngine) Init()");
  your_ref_to_data = data;
  if (your_ref_to_data == NULL)
    return;

  GameData& data_ref = *your_ref_to_data;
  auto& r = data_ref.r;
  // auto& phys = data_ref.world_id;

  // auto world_id = data_ref.world_id;

  // const auto str = std::format("(GameEngine-init) counter {}", data_ref.counter);
  // SDL_Log("%s", str.c_str());
};

void
game_update()
{
  if (!your_ref_to_data)
    return;

  auto& r = your_ref_to_data->r;

  // const auto info = std::format("(GameEngine) Transforms: {}", view.size());
  // SDL_Log("%s", info.c_str());
};

void
game_refresh(GameData* data)
{
  SDL_Log("(GameEngine) game_refresh()");
  your_ref_to_data = data;
  if (!your_ref_to_data)
    return;
  GameData& data_ref = *your_ref_to_data;
  entt::registry& r = your_ref_to_data->r;
  auto view = r.view<const TransformComponent>();
  const auto info = std::format("(GameEngine) Transforms: {}", view.size());
  SDL_Log("%s", info.c_str());
}

} // namespace game2d