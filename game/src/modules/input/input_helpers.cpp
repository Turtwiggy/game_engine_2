#include "pch.hpp" // IWYU pragma: keep

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
  inputs.keys_down.clear();
  inputs.keys_up.clear();

  for (const SDL_Event& evt : events) {

    if (evt.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
      inputs.keys_down.clear();
      inputs.keys_up.clear();
      inputs.keys_held.clear();
      break;
    }

    if (evt.type == SDL_EVENT_KEY_DOWN) {
      const auto scancode = evt.key.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = evt.key.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyDown %s %i %i", scancode_name, down, repeat);

      if (evt.key.repeat)
        continue;
      inputs.keys_down.push_back(scancode);
      inputs.keys_held.emplace(scancode);
    }

    if (evt.type == SDL_EVENT_KEY_UP) {
      const SDL_KeyboardEvent& k_evt = evt.key;
      const auto scancode = k_evt.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = k_evt.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyUp %s %i %i", scancode_name, down, repeat);

      if (evt.key.repeat)
        continue;
      inputs.keys_up.push_back(scancode);

      const auto match = [scancode](SDL_Scancode held_scancode) { return held_scancode == scancode; };
      std::erase_if(inputs.keys_held, match);
    }

    //
  }

  //
};

} // namespace game2d