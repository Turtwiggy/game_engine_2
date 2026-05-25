#pragma once

#include "modules/maths/vec.hpp"
#include "singleton.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

#include <unordered_set>
#include <vector>

namespace game2d {

struct SINGLE_Inputs : Singleton<SINGLE_Inputs>
{
  // keys pressed in Update() to be processed in FixedUpdate()
  // note: this is deliberately not a Set incase a user e.g.
  // press W, release W, press W before one FixedUpdate() occurs.
  std::vector<SDL_Scancode> keys_down;
  std::vector<SDL_Scancode> keys_up;
  std::unordered_set<SDL_Scancode> keys_held;

  vec2 mouse_dt = { 0.0f, 0.0f };

  int n_joysticks = 0;
};

bool
get_key_down(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

bool
get_key_up(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

bool
get_key_held(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode);

void
generate_button_state(const std::vector<SDL_Event>& events, SINGLE_Inputs& inputs);

} // namespace game2d