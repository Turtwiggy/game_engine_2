#include "core/pch.hpp"

#include "core/maths/mat.hpp"
#include "renderer_helpers.hpp"
#include "sdl/sdl_exception.hpp"
#include "sdl/sdl_shader.hpp"
#include "sdl/sdl_surface.hpp"

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
    SDL_CreateGPUSampler(device, &s0), SDL_CreateGPUSampler(device, &s1), SDL_CreateGPUSampler(device, &s2),
    SDL_CreateGPUSampler(device, &s3), SDL_CreateGPUSampler(device, &s4), SDL_CreateGPUSampler(device, &s5),
  };

  /*
const Uint32 vertex_data_mem_size = sizeof(VertexFinal) * vertex_data.size();
const Uint32 index_data_mem_size = sizeof(Index) * index_data.size();

// Create the vertex buffer
const auto vertex_buffer_info = (SDL_GPUBufferCreateInfo){
  .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
  .size = vertex_data_mem_size,
};
auto vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
if (!vertex_buffer)
  throw SDLException("Failed to create GpuBuffer");
SDL_SetGPUBufferName(device, vertex_buffer, "VertexBuffer");

// Create an index buffer
const auto index_buffer_info = (SDL_GPUBufferCreateInfo){
  .usage = SDL_GPU_BUFFERUSAGE_INDEX,
  .size = index_data_mem_size,
};
auto index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
if (!index_buffer)
  throw SDLException("Failed to create GpuBuffer");
SDL_SetGPUBufferName(device, index_buffer, "IndexBuffer");

//
// To get data in to the vertex buffer, we have to use a transfer buffer.
//
const auto transfer_buffer_info = (SDL_GPUTransferBufferCreateInfo){
  .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
  .size = vertex_data_mem_size + index_data_mem_size,
};

// Map the buffer in to cpu memory
auto* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
if (!transfer_buffer)
  throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

// Copy the data in

auto* ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

std::span vertex_buffer_data = { reinterpret_cast<VertexFinal*>(ptr), vertex_data.size() };
std::ranges::copy(vertex_data, vertex_buffer_data.begin());

std::span index_buffer_data = { reinterpret_cast<Index*>(ptr + vertex_data_mem_size), index_data.size() };
std::ranges::copy(index_data, index_buffer_data.begin());

SDL_UnmapGPUTransferBuffer(device, transfer_buffer); // note: need to unmap before aquire gpu command

*/

  //
}

TextureOut
create_texture(SDL_GPUDevice* device, const std::string path)
{
  // Load an image.
  // SDL_Surface* image_data = LoadBMP("ravioli.bmp", 4);
  auto image_data = LoadIMG(path.c_str());

  if (image_data.surface == nullptr || image_data.data == nullptr) {
    throw SDLException("Failed to load image.");
    exit(SDL_APP_FAILURE); // explode
  }

  // Create a texture
  SDL_GPUTextureCreateInfo texture_create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = static_cast<Uint32>(image_data.surface->w),
    .height = static_cast<Uint32>(image_data.surface->h),
    .layer_count_or_depth = 1,
    .num_levels = 1,
  };

  auto* Texture = SDL_CreateGPUTexture(device, &texture_create_info);
  if (!Texture) {
    throw SDLException("Failed to CreateGPUTexture()");
    exit(SDL_APP_FAILURE); // crash
  }
  SDL_SetGPUTextureName(device, Texture, path.c_str());

  // Start texture transfer buffer
  const auto texture_transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = (Uint32)(image_data.surface->w * image_data.surface->h * 4),
  };

  // Map the buffer in to cpu memory
  auto* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_create_info);
  if (!texture_transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  // Copy data in to transfer buffer
  auto* texture_ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
  SDL_memcpy(texture_ptr, image_data.surface->pixels, image_data.surface->w * image_data.surface->h * 4);
  SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);

  SDL_GPUTextureTransferInfo transfer_info = {};
  transfer_info.offset = 0; /* zero out */
  transfer_info.transfer_buffer = texture_transfer_buffer;

  SDL_GPUTextureRegion texture_region = {};
  texture_region.texture = Texture;
  texture_region.x = (Uint32)0;
  texture_region.y = (Uint32)0;
  texture_region.w = (Uint32)texture_create_info.width;
  texture_region.h = (Uint32)texture_create_info.height;
  texture_region.d = 1;

  // Upload.
  auto* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
  auto* copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);
  SDL_UploadToGPUTexture(copy_pass, &transfer_info, &texture_region, false);
  SDL_EndGPUCopyPass(copy_pass);
  if (!SDL_SubmitGPUCommandBuffer(upload_cmd_buf))
    throw SDLException("Unable to SDL_SubmitGPUCommandBuffer()");

  SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);

  TextureOut out;
  out.texture = Texture;
  out.w = image_data.surface->w;
  out.h = image_data.surface->h;

  // note: data is not copied.
  // must be freed in the order:
  SDL_DestroySurface(image_data.surface);
  stbi_image_free(image_data.data);

  return out;
};

SDL_GPUGraphicsPipeline*
create_pipeline(SDL_GPUDevice* device, SDL_Window* window, const ShaderInput& vert, const ShaderInput& frag)
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

  const char* SamplerNames[] = {
    "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  };

  // using VertexFinal = PositionTextureVertex;

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

  // const std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_descriptions{
  //   {
  //     .slot = 0,
  //     .pitch = sizeof(VertexFinal),
  //     .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
  //     .instance_step_rate = 0,
  //   },
  // };

  // Setup to match the vertex shader layout
  // const std::vector<SDL_GPUVertexAttribute> vertex_attributes{
  //   {
  //     // xyz is 3 floats
  //     .location = 0,
  //     .buffer_slot = 0,
  //     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
  //     .offset = 0,
  //   },
  //   {
  //     // uv is a 2 floats
  //     .location = 1,
  //     .buffer_slot = 0,
  //     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
  //     .offset = offsetof(VertexFinal, u),
  //   },
  // };

  // Create the pipelines.
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
      .vertex_shader = vert_shader,
      .fragment_shader = frag_shader,
      // .vertex_input_state = (SDL_GPUVertexInputState){
      //   .vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
      //   .num_vertex_buffers = (Uint32)vertex_buffer_descriptions.size(),
      //   .vertex_attributes = vertex_attributes.data(),
      // 	.num_vertex_attributes = (Uint32)vertex_attributes.size(),
      // },
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info =
          {
            .color_target_descriptions = color_target_desc.data(),
            .num_color_targets = (Uint32)color_target_desc.size(),
          },
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