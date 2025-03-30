#pragma once

#include "common.hpp"

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

//
// hotreload behaviour
// inspired by:
// https://gist.github.com/chrisdill/291c938605c200d079a88d0a7855f31a
//

namespace game2d {

// Functions we call from the dll
typedef void (*game_init_func_t)(GameData* data);
typedef void (*game_fixed_update_func_t)(GameData* data);
typedef void (*game_update_func_t)(GameData* data);
typedef void (*game_update_ui_func_t)(GameUIData* data);
typedef void (*game_refresh_func_t)(GameData* data);

typedef struct sdl_game_code sdl_game_code;
struct sdl_game_code
{
  SDL_SharedObject* game_code_dll;

  game_init_func_t game_init;
  game_fixed_update_func_t game_fixed_update;
  game_update_func_t game_update;
  game_update_ui_func_t game_update_ui;
  game_refresh_func_t game_refresh;

  bool valid = false;
};

sdl_game_code
sdl_load_game_code(const std::string src_dll_name, const std::string dst_dll_name);

void
sdl_unload_game_code(sdl_game_code* game_code);

} // namespace game2d