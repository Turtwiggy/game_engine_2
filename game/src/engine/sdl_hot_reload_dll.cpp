#include "sdl_hot_reload_dll.hpp"
#include "sdl_exception.hpp"

#include <SDL3/SDL.h>

#include <format>

namespace game2d {

// Stubs for when we the dll functions are not loaded
void
game_init_stub(GameData* data) {};
void
game_fixed_update_stub(void) {};
void
game_update_stub(void) {};
void
game_update_ui_stub(GameUIData* ui_data) {};
void
game_refresh_stub(GameData* data) {};

bool
copy_file(const char* src_dll_name, const char* dst_dll_name)
{
  SDL_IOStream* src = SDL_IOFromFile(src_dll_name, "r");
  SDL_IOStream* dst = SDL_IOFromFile(dst_dll_name, "w");

  // Read source into buffer
  auto size = SDL_GetIOSize(src);
  void* buffer = SDL_calloc(1, size);
  SDL_ReadIO(src, buffer, size);
  SDL_WriteIO(dst, buffer, size);

  SDL_CloseIO(src);
  SDL_CloseIO(dst);
  SDL_free(buffer);

  return true;
};

sdl_game_code
sdl_load_game_code(const std::string src_dll_name, const std::string dst_dll_name)
{
  sdl_game_code result = { 0 };

  auto copy_result = copy_file(src_dll_name.c_str(), dst_dll_name.c_str());
  if (!copy_result) {
    throw SDLException("Failed to copy dll.");
    exit(SDL_APP_FAILURE);
  }
  SDL_Log("DLL copied...");

  result.game_code_dll = SDL_LoadObject(dst_dll_name.c_str());
  if (result.game_code_dll == NULL) {
    throw SDLException("Failed to load dll.");
    exit(SDL_APP_FAILURE);
  }

  auto init = (void (*)(GameData*))SDL_LoadFunction(result.game_code_dll, "game_init");
  if (init == NULL) {
    throw SDLException("Failed to load game_init() from dll");
    exit(SDL_APP_FAILURE);
  }
  result.game_init = init;

  auto fixed_update = (void (*)())SDL_LoadFunction(result.game_code_dll, "game_fixed_update");
  if (fixed_update == NULL) {
    throw SDLException("Failed to load game_fixed_update() from dll");
    exit(SDL_APP_FAILURE);
  }
  result.game_fixed_update = fixed_update;

  auto update = (void (*)())SDL_LoadFunction(result.game_code_dll, "game_update");
  if (init == NULL) {
    throw SDLException("Failed to load game_update() from dll");
    exit(SDL_APP_FAILURE);
  }
  result.game_update = update;

  auto update_ui = (void (*)(GameUIData*))SDL_LoadFunction(result.game_code_dll, "game_update_ui");
  if (!update_ui) {
    throw SDLException("Failed to load game_update_ui() from dll");
    exit(SDL_APP_FAILURE);
  }
  result.game_update_ui = update_ui;

  auto refresh = (void (*)(GameData*))SDL_LoadFunction(result.game_code_dll, "game_refresh");
  if (!refresh) {
    throw SDLException("Failed to load game_refresh() from dll");
    exit(SDL_APP_FAILURE);
  }
  result.game_refresh = refresh;

  result.is_valid = true;

  SDL_Log("Load DLL... success");
  return result;
};

void
sdl_unload_game_code(sdl_game_code* game_code)
{
  game_code->is_valid = false;

  if (game_code->game_code_dll) {
    SDL_Log("Unload DLL...");
    SDL_UnloadObject(game_code->game_code_dll);
    game_code->game_code_dll = NULL;
  }

  game_code->game_init = game_init_stub;
  game_code->game_fixed_update = game_fixed_update_stub;
  game_code->game_update = game_update_stub;
  game_code->game_update_ui = game_update_ui_stub;
  game_code->game_refresh = game_refresh_stub;
};

} // namespace game2d