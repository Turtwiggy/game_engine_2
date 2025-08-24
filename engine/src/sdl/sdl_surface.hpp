#pragma once

#include <SDL3/SDL_surface.h>

namespace game2d {

SDL_Surface*
LoadBMP(const char* imageFilename, int desiredChannels);

// Apparently,
// .jpg, .jpeg, .png, .bmp
SDL_Surface*
LoadIMG(const char* filename);

};