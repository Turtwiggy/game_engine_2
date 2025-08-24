#pragma once

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
  SDL_Surface* image_data = nullptr;
  SDL_GPUTexture* texture = nullptr;
  SDL_GPUTransferBuffer* texture_transfer_buffer = nullptr;
};
TextureOut
create_texture(SDL_GPUDevice* device, const std::string path);

} // namespace game2d