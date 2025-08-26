#pragma once

#include "sdl/sdl_shader.hpp"

#include <SDL3/SDL_gpu.h>
#include <entt/fwd.hpp>

namespace game2d {

struct RendererInfo
{
  SDL_GPUDevice* device;
  SDL_Window* window;

  //  "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  std::vector<SDL_GPUSampler*> samplers;
};
void
setup_renderer(RendererInfo& ri);

struct TextureOut
{
  int w = 0;
  int h = 0;
  SDL_GPUTexture* texture = nullptr;
};

/*
  https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#example-for-sdl_gpu-users
*/
TextureOut
create_texture(SDL_GPUDevice* device, const std::string path);

SDL_GPUGraphicsPipeline*
create_pipeline(SDL_GPUDevice* device, SDL_Window* window, const ShaderInput& vert, const ShaderInput& frag);

} // namespace game2d