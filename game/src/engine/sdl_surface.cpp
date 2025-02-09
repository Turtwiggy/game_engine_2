#include "sdl_surface.hpp"

#include "sdl_exception.hpp"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>

#if !defined(STB_IMAGE_IMPLEMENTATION)
#define STB_IMAGE_IMPLEMENTATION
#endif
#include <stb_image.h>

#include <format>
#include <stdexcept>

namespace game2d {

SDL_Surface*
LoadBMP(const char* imageFilename, int desiredChannels)
{
  auto BasePath = SDL_GetBasePath();

  char fullPath[256];
  SDL_Surface* result;
  SDL_PixelFormat format;
  SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/textures/%s", BasePath, imageFilename);

  result = SDL_LoadBMP(fullPath);
  if (result == NULL) {
    throw SDLException("Failed to load BMP!");
    return NULL;
  }

  if (desiredChannels == 4) {
    format = SDL_PIXELFORMAT_ABGR8888;
  } else {
    throw std::runtime_error("Unexpected number of channnels (no alpha?)");
    SDL_DestroySurface(result);
    return NULL;
  }

  if (result->format != format) {
    SDL_Surface* next = SDL_ConvertSurface(result, format);
    SDL_DestroySurface(result);
    result = next;
  }

  return result;
};

SDL_Surface*
LoadIMG(const char* filename)
{
  auto base_path = SDL_GetBasePath();
  char full_path[256];
  SDL_snprintf(full_path, sizeof(full_path), "%sassets/textures/%s", base_path, filename);

  int w = 0;
  int h = 0;
  int c = 0;
  unsigned char* data = stbi_load(full_path, &w, &h, &c, 0);
  if (!data) {
    auto err = std::format("Unable to find image: {}", full_path);
    throw std::runtime_error(err);
    stbi_image_free(data);
    return nullptr;
  }

  SDL_PixelFormat format;
  if (c == 4)
    format = SDL_PIXELFORMAT_ABGR8888;
  else if (c == 3)
    format = SDL_PIXELFORMAT_RGB24;
  else {
    stbi_image_free(data);
    auto err = std::format("Unsupported number of channels: {}", c);
    throw std::runtime_error(err);
    return NULL;
  }

  // Pitch is the offset in bytes from one row of pixels to the next,
  // e.g. width*4 for SDL_PIXELFORMAT_RGBA8888.
  const int pitch = w * 4;

  SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, format, data, pitch);
  if (!surface) {
    auto err = std::format("Unable to SDL_CreateSurfaceFrom(), file: {}", full_path);
    throw SDLException(err);
    return NULL;
  }

  // note: data is not copied.
  // must be freed in the order:
  // SDL_DestroySurface(surface);
  // stbi_image_free(data);

  return surface;
};

} // namespace game2d