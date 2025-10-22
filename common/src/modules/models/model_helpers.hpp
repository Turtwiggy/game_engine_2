#pragma once

#include "model_components.hpp"

#include "modules/sdl/sdl_shader.hpp"

namespace game2d {

Model
load_model(SDL_GPUDevice* device, const std::string path);

SDL_GPUGraphicsPipeline*
create_model_pipeline(SDL_GPUDevice* device,
                      SDL_Window* window,
                      const ShaderInput& vert,
                      const ShaderInput& frag,
                      const SDL_GPUSampleCount sample_count);

} // namespace game2d