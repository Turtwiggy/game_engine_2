#pragma once

#include "game_and_engine_interop.hpp"

#include <SDL3/SDL.h>
#include <entt/entt.hpp>

namespace game2d {

// Functions we call from the dll
typedef void (*game_init_func_t)(GameData* data);
typedef void (*game_fixed_update_func_t)(GameData* data);
typedef void (*game_update_func_t)(GameData* data);
typedef void (*game_update_ui_func_t)(GameUIData* data);
typedef void (*game_refresh_func_t)(GameData* data);
typedef void (*game_shutdown_func_t)(GameData* data);

typedef struct sdl_game_code sdl_game_code;
struct sdl_game_code
{
  SDL_SharedObject* game_code_dll;

  game_init_func_t game_init;
  game_fixed_update_func_t game_fixed_update;
  game_update_func_t game_update;
  game_update_ui_func_t game_update_ui;
  game_refresh_func_t game_refresh;
  game_shutdown_func_t game_shutdown;

  std::atomic_bool valid = false;
  std::atomic_bool rebuilt = false;
};

void
sdl_load_game_code(sdl_game_code& result, const std::string src_dll_name, const std::string dst_dll_name);

void
sdl_unload_game_code(sdl_game_code& game_code);

} // namespace game2d