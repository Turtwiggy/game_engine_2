#pragma once

#include <SDL3/SDL_error.h>

#include <stdexcept>

class SDLException final : public std::runtime_error
{
public:
  explicit SDLException(const std::string& message)
    : std::runtime_error(message + '\n' + SDL_GetError()) {};
};