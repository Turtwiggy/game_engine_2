#include "game.hpp"
#include "box2d/math_functions.h"

#include <SDL3/SDL_log.h>
#include <box2d/box2d.h>
#include <imgui.h>

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

bool refreshed = false;

void
game_fixed_update()
{
  if (!your_ref_to_data)
    return;
  auto& r = your_ref_to_data->r;

  if (refreshed) {
    refreshed = false;
    SDL_Log("FixedUpdate() Refresh()");

    auto view = r.view<const PhysicsBodyComponent, const TransformComponent>();
    SDL_Log("FixedUpdate stuff: %zu", view.size_hint());

    for (const auto& [e, pb_c, t_c] : view.each()) {
      // SDL_Log("pos %f %f", t_c.pos.x, t_c.pos.y);

      static RandomState rnd(69);
      const float rnd_x = random(rnd, 0, 640);
      const float rnd_y = random(rnd, 0, 360);
      // b2Body_GetPosition(pb_c.id);
      b2Body_SetTransform(pb_c.id, pixels_to_meters({ rnd_x, rnd_y }), b2Rot_identity);
    }
  }
}

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
game_update_ui(GameUIData* ui_data)
{
  ImGui::SetCurrentContext(ui_data->ctx);

  auto flags = 0;
  flags |= ImGuiWindowFlags_NoDecoration;

  ImGui::SetNextWindowSize({ 100.0f, 100.0f });

  ImGui::Begin("SomeCrazyWindow", nullptr, flags);
  ImGui::Text("wow das crazy!");
  ImGui::End();
};

void
game_refresh(GameData* data)
{
  SDL_Log("(GameEngine) game_refresh()");
  your_ref_to_data = data;
  if (!your_ref_to_data)
    return;
  refreshed = true;

  GameData& data_ref = *your_ref_to_data;
  entt::registry& r = your_ref_to_data->r;
  auto view = r.view<const TransformComponent>();
  const auto info = std::format("(GameEngine) Transforms: {}", view.size());
  SDL_Log("%s", info.c_str());
};

} // namespace game2d