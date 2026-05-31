#include "pch.hpp"

#include "modules/sdl/sdl_exception.hpp"
#include "modules/sdl/sdl_shader.hpp"
#include "renderer_helpers.hpp"

namespace game2d {

void
setup_renderer(RendererInfo& ri)
{
  auto* device = ri.device;
  auto* window = ri.window;

  game2d::InitializeAssetLoader();

  SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
  if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
    present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
  else if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_MAILBOX))
    present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
  SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode);

  const auto s0 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  const auto s1 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };
  const auto s2 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  const auto s3 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };
  const auto s4 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .max_anisotropy = 4,
    .enable_anisotropy = true,
  };
  const auto s5 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .max_anisotropy = 4,
    .enable_anisotropy = true,
  };

  //  "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  ri.samplers = std::vector{
    SDL_CreateGPUSampler(device, &s0), //
    SDL_CreateGPUSampler(device, &s1), //
    SDL_CreateGPUSampler(device, &s2), //
    SDL_CreateGPUSampler(device, &s3), //
    SDL_CreateGPUSampler(device, &s4), //
    SDL_CreateGPUSampler(device, &s5), //
  };
};

struct TextureOutA
{
  int w = 0;
  int h = 0;
  SDL_GPUTexture* texture = nullptr;
  unsigned char* data = nullptr;
};
TextureOutA
create_texture(SDL_GPUDevice* device, const std::string path)
{
  // Load an image.
  auto base_path = SDL_GetBasePath();
  char full_path[256];
  SDL_snprintf(full_path, sizeof(full_path), "%sassets/%s", base_path, path.c_str());
  SDL_Log("Loading image: %s", full_path);

  int w = 0;
  int h = 0;
  int c = 0;
  unsigned char* data = stbi_load(full_path, &w, &h, &c, 0);
  if (data == NULL) {
    auto err = std::format("Unable to find image: {}", full_path);
    SDL_Log("%s", err.c_str());
    throw std::runtime_error(err);
  }

  if (c < 4)
    throw std::runtime_error(std::format("Unsupported number of channels {}", c));

  // Create texture
  SDL_GPUTextureCreateInfo texture_create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = (uint32_t)w,
    .height = (uint32_t)h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  };

  SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texture_create_info);
  if (!texture) {
    auto err = SDL_GetError();
    SDL_Log("%s", err);
    throw SDLException("Failed to CreateGPUTexture()");
    exit(SDL_APP_FAILURE); // crash
  }

  const TextureOutA out{
    .w = w,
    .h = h,
    .texture = texture,
    .data = data,
  };
  return out;
};

SDL_GPUTextureFormat
get_depth_stencil_format(SDL_GPUDevice* device)
{
  if (SDL_GPUTextureSupportsFormat(
        device, SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;

  if (SDL_GPUTextureSupportsFormat(
        device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    return SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;

  SDL_Log("Stencil formats not supported!");
  exit(SDL_APP_FAILURE); // crash
  // return depth_stencil_format;
}

SDL_GPUTexture*
create_depth_texture(SDL_GPUDevice* device, int w, int h, const SDL_GPUSampleCount sample_count)
{
  SDL_GPUTextureCreateInfo texture_create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = get_depth_stencil_format(device),
    .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
    .width = (uint32_t)w,
    .height = (uint32_t)h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .sample_count = sample_count,
  };

  auto* texture = SDL_CreateGPUTexture(device, &texture_create_info);
  if (!texture) {
    throw SDLException("Failed to CreateGPUTexture()");
    exit(SDL_APP_FAILURE); // crash
  }

  return texture;
}

void
upload_texture_to_gpu(SDL_GPUDevice* device, TextureOutA& tex_in)
{
  const auto w = tex_in.w;
  const auto h = tex_in.h;
  auto* texture = tex_in.texture;
  auto* data = tex_in.data;
  if (!data)
    throw std::runtime_error("texture data is null");

  // SDL_SetGPUTextureName(device, texture, path.c_str());

  // Start texture transfer buffer
  const auto texture_transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = (Uint32)(w * h * 4),
  };

  // Map the buffer in to cpu memory
  auto* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_create_info);
  if (!texture_transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  // Copy data in to transfer buffer
  auto* texture_transfer_ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
  SDL_memcpy(texture_transfer_ptr, data, w * h * 4);
  SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);

  SDL_GPUTextureTransferInfo transfer_info = {};
  transfer_info.offset = 0; /* zero out */
  transfer_info.transfer_buffer = texture_transfer_buffer;

  const SDL_GPUTextureRegion texture_region = {
    .texture = texture,
    .x = (Uint32)0,
    .y = (Uint32)0,
    .w = (Uint32)w,
    .h = (Uint32)h,
    .d = 1,
  };

  // Upload.
  auto* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
  auto* copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);
  SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
  SDL_EndGPUCopyPass(copy_pass);

  if (!SDL_SubmitGPUCommandBuffer(upload_cmd_buf))
    throw SDLException("Unable to SDL_SubmitGPUCommandBuffer()");

  SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);
};

TextureOutB
create_and_upload_gpu_texture(SDL_GPUDevice* device, const std::string path)
{
  auto texture = create_texture(device, path);

  upload_texture_to_gpu(device, texture);

  // note: data is not copied.
  // must be freed in the order:
  stbi_image_free(texture.data);

  return { .w = texture.w, .h = texture.h, .texture = texture.texture };
};

SDL_GPUGraphicsPipeline*
create_2d_pipeline(SDL_GPUDevice* device,
                   SDL_Window* window,
                   const ShaderInput& vert,
                   const ShaderInput& frag,
                   const SDL_GPUSampleCount sample_count)
{
  SDL_GPUShader* vert_shader = nullptr;
  SDL_GPUShader* frag_shader = nullptr;

  vert_shader = game2d::LoadShader(device, vert);
  if (vert_shader == NULL) {
    SDL_Log("Failed to create vert shader");
    exit(SDL_APP_FAILURE); // explode
  };

  frag_shader = game2d::LoadShader(device, frag);
  if (frag_shader == NULL) {
    SDL_Log("Failed to create frag shader");
    exit(SDL_APP_FAILURE); // explode
  };

  const std::vector<SDL_GPUColorTargetDescription> color_target_desc{
      {.format = SDL_GetGPUSwapchainTextureFormat(device, window),
       .blend_state =
           {
               .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
               .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
               .color_blend_op = SDL_GPU_BLENDOP_ADD,
               .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
               .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
               .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
               .enable_blend = true,
           }},
  };

  // Create the pipelines.
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vert_shader,
    .fragment_shader = frag_shader,
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .multisample_state = {
      .sample_count = sample_count,
    },
    // .depth_stencil_state = {
    //     .compare_op = SDL_GPU_COMPAREOP_LESS,
    //     .enable_depth_test = true,
    //     .enable_depth_write = true,
    // },
    .target_info = { .color_target_descriptions = color_target_desc.data(),
                     .num_color_targets = (Uint32)color_target_desc.size(),
                    }
  };

  // pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
  pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  SDL_GPUGraphicsPipeline* fill_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
  if (fill_pipeline == nullptr) {
    throw SDLException("Failed to create Fill GraphicsPipeline()");
    exit(SDL_APP_FAILURE); // crash
  }

  // can release shaders after creating pipelines
  SDL_Log("Releasing shaders... be free!");
  SDL_ReleaseGPUShader(device, vert_shader);
  SDL_ReleaseGPUShader(device, frag_shader);

  return fill_pipeline;
};

} // namespace game2d