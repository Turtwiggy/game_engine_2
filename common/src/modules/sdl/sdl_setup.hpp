#pragma once

namespace game2d {

void
setup_sdl();

void
setup_sdl_controllers();

SDL_GPUDevice*
setup_sdl_gpu();

SDL_Window*
setup_sdl_window(float main_scale, int w, int h);

} // namespace game2d