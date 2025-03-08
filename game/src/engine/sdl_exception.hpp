#pragma once

#include <SDL3/SDL_error.h>

#include <stdexcept>

namespace game2d {

class SDLException final : public std::runtime_error
{
public:
  explicit SDLException(const std::string& message)
    : std::runtime_error(message + '\n' + SDL_GetError()) {};
};

} // namespace game2d