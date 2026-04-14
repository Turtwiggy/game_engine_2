#pragma once

#include "modules/sdl/sdl_shader.hpp"

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

struct TextureOutB
{
  int w = 0;
  int h = 0;
  SDL_GPUTexture* texture = nullptr;
};
TextureOutB
create_and_upload_gpu_texture(SDL_GPUDevice* device, const std::string path);

SDL_GPUTextureFormat
get_depth_stencil_format(SDL_GPUDevice* device);

SDL_GPUTexture*
create_depth_texture(SDL_GPUDevice* device, int w, int h, const SDL_GPUSampleCount sample_count);

SDL_GPUGraphicsPipeline*
create_2d_pipeline(SDL_GPUDevice* device,
                   SDL_Window* window,
                   const ShaderInput& vert,
                   const ShaderInput& frag,
                   const SDL_GPUSampleCount msaa);

// SDL_GPUGraphicsPipeline*
// create_3d_pipeline(SDL_GPUDevice* device,
//                    SDL_Window* window,
//                    const ShaderInput& vert,
//                    const ShaderInput& frag,
//                    const SDL_GPUSampleCount sample_count);

} // namespace game2d