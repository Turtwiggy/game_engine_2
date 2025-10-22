#include "pch.hpp"
#include <SDL3/SDL_gpu.h>

#include "imgui_helpers.hpp"

namespace game2d {

void
setup_imgui(ImGuiSetup& in)
{
  auto* game_ui_data = in.game_ui_data;
  auto main_scale = in.main_scale;
  auto* window = in.window;
  auto* device = in.device;

  // clang-format off

  IMGUI_CHECKVERSION();
  game_ui_data->ctx = ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  ImGui::StyleColorsDark();
  
  // Setup scaling
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
  style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
  io.ConfigDpiScaleFonts = true;          // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
  io.ConfigDpiScaleViewports = true;      // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

  // clang-format on

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLGPU(window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = device;
  init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1; // Only used in multi-viewports mode.
  ImGui_ImplSDLGPU3_Init(&init_info);
}

void
cleanup_imgui(SDL_GPUDevice* device)
{
  SDL_WaitForGPUIdle(device);
  ImGui_ImplSDL3_Shutdown();
  ImGui_ImplSDLGPU3_Shutdown();
  ImGui::DestroyContext();
}

} // namespace game2d