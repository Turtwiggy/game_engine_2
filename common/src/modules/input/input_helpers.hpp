#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <entt/fwd.hpp>
#include <unordered_set>

namespace game2d {

struct SINGLE_Inputs
{
  std::unordered_set<SDL_Scancode> keys_down;
  std::unordered_set<SDL_Scancode> keys_up;
  std::unordered_set<SDL_Scancode> keys_held;
};

bool
get_key_down(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

bool
get_key_up(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

// note: not properly implemented yet
bool
get_key_held(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

void
generate_button_state(const std::vector<SDL_Event>& events, SINGLE_Inputs& inputs);

} // namespace game2d