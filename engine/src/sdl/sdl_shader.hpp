#pragma once

#include <SDL3/SDL_gpu.h>

namespace game2d {

void
InitializeAssetLoader();

SDL_GPUShader*
LoadShader(SDL_GPUDevice* device,
           const char* shaderFilename,
           const Uint32 samplerCount,
           const Uint32 uniformBufferCount,
           const Uint32 storageBufferCount,
           const Uint32 storageTextureCount);

} // namespace game2d