#include "core/pch.hpp"

#include "core/maths/mat.hpp"
#include "renderer_helpers.hpp"
#include "sdl/sdl_exception.hpp"
#include "sdl/sdl_shader.hpp"
#include "sdl/sdl_surface.hpp"
#include <SDL3/SDL_gpu.h>

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
  SDL_Surface* image_data = LoadIMG(path.c_str());
  if (!image_data) {
    throw SDLException("Failed to load image.");
    exit(SDL_APP_FAILURE); // explode
  }

  // Create a texture
  SDL_GPUTextureCreateInfo texture_create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = static_cast<Uint32>(image_data->w),
    .height = static_cast<Uint32>(image_data->h),
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
    .size = (Uint32)(image_data->w * image_data->h * 4),
  };

  // Map the buffer in to cpu memory
  auto* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_create_info);
  if (!texture_transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  // Copy data in to transfer buffer
  auto* texture_ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
  SDL_memcpy(texture_ptr, image_data->pixels, image_data->w * image_data->h * 4);
  SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);

  TextureOut out;
  out.image_data = image_data;
  out.texture = Texture;
  out.texture_transfer_buffer = texture_transfer_buffer;
  return out;
}

} // namespace game2d