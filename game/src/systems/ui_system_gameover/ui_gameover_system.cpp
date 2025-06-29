#include "core/pch.hpp"

#include "ui_gameover_components.hpp"

#include "ui_gameover_system.hpp"

namespace game2d {

void
update_ui_gameover_system(CommonUiData& data)
{
  // #if defined(_DEBUG)
  //   ZoneScoped;
  // #endif
  // const auto game_over_view = r.view<Request_GameOver>();

  if (!data.game_over)
    return;

  // TODO: fix this being hard coded
  constexpr int SDL_WINDOW_WIDTH = 1280;
  constexpr int SDL_WINDOW_HEIGHT = 720;
  const auto wh = ImVec2{ SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT };
  const auto ui_center = ImVec2{ wh.x * 0.5f, wh.y * 0.5f };
  ImGui::SetNextWindowPos(ui_center, ImGuiCond_Always, ImVec2{ 0.5f, 0.5f });

  ImGuiWindowFlags flags = 0;
  flags |= ImGuiWindowFlags_NoDecoration;
  flags |= ImGuiWindowFlags_NoDocking;
  flags |= ImGuiWindowFlags_NoMove;
  flags |= ImGuiWindowFlags_AlwaysAutoResize;
  flags |= ImGuiWindowFlags_NoSavedSettings;
  // flags |= ImGuiWindowFlags_NoBackground;

  ImGui::Begin("Gameover", NULL, flags);
  ImGui::Text("gameover is over, fam!");

  if (ImGui::Button("Play again")) {
    data.play_again = true;
    SDL_Log("(ui-thread) ui clicked to play again");
  }

  ImGui::End();
}

} // namespace game2d