#pragma once

#include <SDL3/SDL_gpu.h>

namespace game2d {

void
InitializeAssetLoader();

struct ShaderInput
{
  const char* shaderFilename;
  const Uint32 samplerCount;
  const Uint32 uniformBufferCount;
  const Uint32 storageBufferCount;
  const Uint32 storageTextureCount;
};
SDL_GPUShader*
LoadShader(SDL_GPUDevice* device, const ShaderInput& input);

} // namespace game2d