#include "pch.hpp"

#include "input_helpers.hpp"

namespace game2d {

bool
get_key_down(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode)
{
  const auto& keys = inputs_c.keys_down;
  auto it = std::find(keys.begin(), keys.end(), scancode);
  return it != keys.end();
};

bool
get_key_up(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode)
{
  const auto& keys = inputs_c.keys_up;
  auto it = std::find(keys.begin(), keys.end(), scancode);
  return it != keys.end();
};

bool
get_key_held(const SINGLE_Inputs& inputs_c, const SDL_Scancode scancode)
{
  const auto& keys = inputs_c.keys_held;
  auto it = std::find(keys.begin(), keys.end(), scancode);
  return it != keys.end();
};

void
generate_button_state(const std::vector<SDL_Event>& events, SINGLE_Inputs& inputs)
{
  // take a copy of all held inputs.
  // todo: implement held state properly

  const auto held = inputs.keys_held;

  inputs.keys_down.clear();
  inputs.keys_up.clear();
  // inputs.keys_held.clear();

  for (const SDL_Event& evt : events) {

    if (evt.type == SDL_EVENT_KEY_DOWN) {
      const auto scancode = evt.key.scancode;
      inputs.keys_down.emplace(scancode);
      inputs.keys_held.emplace(scancode);
    }

    if (evt.type == SDL_EVENT_KEY_UP) {
      const auto scancode = evt.key.scancode;
      inputs.keys_up.emplace(scancode);
      if (inputs.keys_held.contains(scancode))
        inputs.keys_held.erase(scancode);
    }

    //
  }
};

} // namespace game2d