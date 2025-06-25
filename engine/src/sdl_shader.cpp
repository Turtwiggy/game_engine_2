#include "core/pch.hpp"

#include "sdl_shader.hpp"

namespace game2d {

void
InitializeAssetLoader()
{
  SDL_GetBasePath();
}

SDL_GPUShader*
LoadShader(SDL_GPUDevice* device,
           const char* shaderFilename,
           const Uint32 samplerCount,
           const Uint32 uniformBufferCount,
           const Uint32 storageBufferCount,
           const Uint32 storageTextureCount)
{
  auto BasePath = SDL_GetBasePath();

  // Auto-detect the shader stage from the file name for convenience
  SDL_GPUShaderStage stage;
  if (SDL_strstr(shaderFilename, ".vert")) {
    stage = SDL_GPU_SHADERSTAGE_VERTEX;
  } else if (SDL_strstr(shaderFilename, ".frag")) {
    stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  } else {
    throw new std::runtime_error("Unrecognized shader stage!");
    exit(SDL_APP_FAILURE); // crash
    return NULL;
  }

  char full_path[256];
  SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
  SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
  const char* entrypoint;

  auto assets = "assets/shaders/compiled";

  if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
    SDL_snprintf(full_path, sizeof(full_path), "%s%s/SPIRV/%s.spv", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_SPIRV;
    entrypoint = "main";
  } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
    SDL_snprintf(full_path, sizeof(full_path), "%s%s/MSL/%s.msl", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_MSL;
    entrypoint = "main0";
  } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
    SDL_snprintf(full_path, sizeof(full_path), "%s%s/DXIL/%s.dxil", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_DXIL;
    entrypoint = "main";
  } else {
    SDL_Log("%s", "Unrecognized backend shader format!");
    return NULL;
  }

  const auto load_str = std::format("Loading shader... {}", full_path);
  SDL_Log("%s", load_str.c_str());

  size_t codeSize;
  void* code = SDL_LoadFile(full_path, &codeSize);
  if (code == NULL) {
    const auto err_str = std::format("Failed to load shader from disk! {}", full_path);
    SDL_Log("%s", err_str.c_str());
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_info = {
    .code_size = codeSize,
    .code = (Uint8*)code,
    .entrypoint = entrypoint,
    .format = format,
    .stage = stage,
    .num_samplers = samplerCount,
    .num_storage_textures = storageTextureCount,
    .num_storage_buffers = storageBufferCount,
    .num_uniform_buffers = uniformBufferCount,
    .props = 0,
  };

  SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
  if (shader == NULL) {
    SDL_Log("%s", "Failed to create shader!");
    SDL_free(code);
    return NULL;
  }

  SDL_free(code);
  return shader;
}

}