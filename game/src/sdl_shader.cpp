#include "sdl_shader.hpp"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>

namespace game2d {

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
    SDL_Log("Invalid shader stage!");
    return NULL;
  }

  char fullPath[256];
  SDL_GPUShaderFormat backendFormats = SDL_GetGPUShaderFormats(device);
  SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
  const char* entrypoint;

  auto assets = "assets/shaders_compiled/";

  if (backendFormats & SDL_GPU_SHADERFORMAT_SPIRV) {
    SDL_snprintf(fullPath, sizeof(fullPath), "%s%s/SPIRV/%s.spv", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_SPIRV;
    entrypoint = "main";
  } else if (backendFormats & SDL_GPU_SHADERFORMAT_MSL) {
    SDL_snprintf(fullPath, sizeof(fullPath), "%s%s/MSL/%s.msl", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_MSL;
    entrypoint = "main0";
  } else if (backendFormats & SDL_GPU_SHADERFORMAT_DXIL) {
    SDL_snprintf(fullPath, sizeof(fullPath), "%s%s/DXIL/%s.dxil", BasePath, assets, shaderFilename);
    format = SDL_GPU_SHADERFORMAT_DXIL;
    entrypoint = "main";
  } else {
    SDL_Log("%s", "Unrecognized backend shader format!");
    return NULL;
  }

  size_t codeSize;
  void* code = SDL_LoadFile(fullPath, &codeSize);
  if (code == NULL) {
    SDL_Log("Failed to load shader from disk! %s", fullPath);
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
    SDL_Log("Failed to create shader!");
    SDL_free(code);
    return NULL;
  }

  SDL_free(code);
  return shader;
}

}