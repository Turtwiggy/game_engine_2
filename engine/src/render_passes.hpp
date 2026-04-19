#pragma once

#include "modules/maths/vec.hpp"
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>

namespace game2d {

struct UniformBlock
{
  float data[4];
};

void
render_to_texture(SDL_GPUCommandBuffer* cmd_buf,
                  SDL_GPUGraphicsPipeline* pipeline,
                  SDL_GPUBuffer* sprite_data_buffer,
                  SDL_GPUTexture* texture,
                  SDL_GPUSampler* sampler,
                  const glm::mat4 vp_matrix,
                  const glm::vec4 clear_col,
                  std::vector<SDL_GPUTexture*> sampled_textures,
                  const uint32_t SPRITE_COUNT,
                  UniformBlock* ubo = nullptr);

void
render_to_swapchain(SDL_GPUCommandBuffer* cmd_buf,
                    SDL_GPUGraphicsPipeline* pipeline,
                    SDL_GPUBuffer* quad_data_buffer,
                    std::vector<SDL_GPUTexture*> textures,
                    SDL_GPUSampler* sampler,
                    SDL_Window* window,
                    ImDrawData* draw_data,
                    glm::mat4 vp_matrix_nopos,
                    const UniformBlock& ubo);

} // namespace game2d