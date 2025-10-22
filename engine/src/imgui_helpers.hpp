#pragma once

#include "game_and_engine_interop.hpp"
#include <SDL3/SDL_gpu.h>

namespace game2d {

struct ImGuiSetup
{
  GameUIData* game_ui_data;
  float main_scale = 1.0f;
  SDL_Window* window;
  SDL_GPUDevice* device;
};

void
setup_imgui(ImGuiSetup& in);

void
cleanup_imgui(SDL_GPUDevice* device);

} // namespace game2d