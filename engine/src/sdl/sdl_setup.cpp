#include "core/pch.hpp"

#include "sdl_setup.hpp"

#include "sdl/sdl_exception.hpp"

namespace game2d {

void
setup_sdl()
{
#if defined(SDL_PLATFORM_WIN32)
  SDL_Log("Hello, Windows!");
#elif defined(SDL_PLATFORM_MACOS)
  SDL_Log("Hello, Mac!");
#elif defined(SDL_PLATFORM_LINUX)
  SDL_Log("Hello, Linux!");
#elif defined(SDL_PLATFORM_IOS)
  SDL_Log("Hello, iOS!");
#elif defined(SDL_PLATFORM_ANDROID);
  SDL_Log("Hello, Android!");
#else
  SDL_Log("Hello, Unknown platform!");
#endif

  SDL_Log("Hello, MainThread!");
  SDL_Log("You have %i logical cpu cores", SDL_GetNumLogicalCPUCores());
  SDL_Log("(main()) SDL_IsMainThread(): %i", SDL_IsMainThread());
  SDL_Log("SDL_Version: %i", SDL_GetVersion());
}

void
setup_sdl_controllers()
{
  int num_joysticks = 0;
  const SDL_JoystickID* joysticks = SDL_GetJoysticks(&num_joysticks);
  for (int i = 0; i < num_joysticks; i++) {
    const SDL_JoystickID id = joysticks[i];
    const auto* name = SDL_GetJoystickNameForID(id);
    SDL_Log("Joystick %d: %s", i, name ? name : "Unknown");

    const auto* instance = SDL_OpenJoystick(id);
    if (instance)
      SDL_Log("Joystick Connected: success");
    else
      SDL_Log("Joystick Connected: fail");
  }
}

SDL_GPUDevice*
setup_sdl_gpu()
{
  SDL_GPUDevice* device;

  auto flags = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL;
#if defined(_DEBUG)
  device = { SDL_CreateGPUDevice(flags, true, nullptr) };
#else
  device = { SDL_CreateGPUDevice(flags, false, nullptr) };
#endif
  if (device == NULL)
    throw SDLException("Couldn't create GPU device");

  return device;
}

SDL_Window*
setup_sdl_window(float main_scale, int w, int h)
{
  const auto window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  const auto scale_x = (int)(w * main_scale);
  const auto scale_y = (int)(h * main_scale);
  SDL_Window* window = SDL_CreateWindow("Game", scale_x, scale_y, window_flags);

  if (window == NULL)
    throw SDLException("Couldn't SDL_CreateWindow()");

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  return window;
}

} // namespace game2d