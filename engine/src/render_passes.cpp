#include "pch.hpp" // IWYU pragma: keep

#include "render_passes.hpp"

namespace game2d {

void
render_to_texture(const RenderToTextureInput& input)
{
  auto* cmd_buf = input.cmd_buf;
  auto* pipeline = input.pipeline;
  auto* sprite_data_buffer = input.data_buffer;
  auto* texture = input.texture;
  auto& vp_matrix = input.vp_matrix;
  auto& clear_col = input.clear_col;
  auto* shared_sampler = input.sampler;
  auto& sampled_textures = input.sampled_textures;
  auto SPRITE_COUNT = input.SPRITE_COUNT;
  auto* uniform = input.ubo;

  const SDL_GPUColorTargetInfo col_info_a = {
    .texture = texture,
    .mip_level = 0,
    .layer_or_depth_plane = 0,
    .clear_color = SDL_FColor{ clear_col.x, clear_col.y, clear_col.z, clear_col.w },
    .load_op = SDL_GPU_LOADOP_CLEAR,
    .store_op = SDL_GPU_STOREOP_STORE,
    .cycle = false,
  };

  SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info_a, 1, NULL);
  {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_BindGPUVertexStorageBuffers(render_pass, 0, &sprite_data_buffer, 1);
    SDL_PushGPUVertexUniformData(cmd_buf, 0, &vp_matrix, sizeof(glm::mat4));

    if (uniform) {
      static_assert(sizeof(UniformBlock) % 16 == 0);
      SDL_PushGPUFragmentUniformData(cmd_buf, 0, uniform, sizeof(UniformBlock));
    }

    std::vector<SDL_GPUTextureSamplerBinding> bindings;
    for (int i = 0; i < sampled_textures.size(); i++) {
      auto sampled_texture = sampled_textures[i];
      bindings.push_back({
        .texture = sampled_texture,
        .sampler = shared_sampler,
      });
    }
    if (!bindings.empty())
      SDL_BindGPUFragmentSamplers(render_pass, 0, bindings.data(), (uint32_t)bindings.size());

    SDL_DrawGPUPrimitives(render_pass, SPRITE_COUNT * 6, 1, 0, 0);
  }
  SDL_EndGPURenderPass(render_pass);
}

void
render_to_swapchain(SDL_GPUCommandBuffer* cmd_buf,
                    SDL_GPUGraphicsPipeline* pipeline,
                    SDL_GPUBuffer* quad_data_buffer,
                    SDL_GPUBuffer* lights_buffer,
                    std::vector<SDL_GPUTexture*> textures,
                    SDL_GPUSampler* sampler,
                    SDL_Window* window,
                    ImDrawData* draw_data,
                    glm::mat4 vp_matrix_nopos,
                    const UniformBlock& ubo)
{
  const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

  // https://wiki.libsdl.org/SDL3/SDL_WaitAndAcquireGPUSwapchainTexture
  SDL_GPUTexture* swapchain_texture;
  SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_texture, nullptr, nullptr);
  if (swapchain_texture != nullptr && !is_minimized) {

    // This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

    // Render the final output to the swapchain_texture
    //
    {
      const SDL_GPUColorTargetInfo col_info = {
        .texture = swapchain_texture,
        .mip_level = 0,
        .layer_or_depth_plane = 0,
        .clear_color = SDL_FColor{ 1.0f, 0.0f, 0.0f, 1.0f },
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
        .cycle = false,
      };

      SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info, 1, NULL);
      {
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
        SDL_BindGPUVertexStorageBuffers(render_pass, 0, &quad_data_buffer, 1);
        SDL_BindGPUFragmentStorageBuffers(render_pass, 0, &lights_buffer, 1);

        if (textures.size() > 0) {
          std::vector<SDL_GPUTextureSamplerBinding> texs;
          std::for_each(textures.begin(), textures.end(), [&](SDL_GPUTexture* texture) {
            texs.push_back({ .texture = texture, .sampler = sampler });
          });
          SDL_BindGPUFragmentSamplers(render_pass, 0, texs.data(), texs.size());
        }

        SDL_PushGPUVertexUniformData(cmd_buf, 0, &vp_matrix_nopos, sizeof(glm::mat4));
        SDL_PushGPUFragmentUniformData(cmd_buf, 0, &ubo, sizeof(UniformBlock));

        // Render one, full screen quad.
        SDL_DrawGPUPrimitives(render_pass, 1 * 6, 1, 0, 0);

        // in the main swapchain texture, call renderdrawdata
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);
      }
      SDL_EndGPURenderPass(render_pass);
    }
  }
}

} // namespace game2d