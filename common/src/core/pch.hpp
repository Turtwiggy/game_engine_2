#pragma once

#ifdef __cplusplus

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <entt/entt.hpp>

// clang-format off
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlgpu3.h>
// #include <backends/imgui_impl_vulkan.h>
// clang-format on

#if defined(_DEBUG)
#if !defined(TRACY_ENABLE)
#define TRACY_ENABLE
#define TRACY_CALLSTACK
#endif
#include <tracy/Tracy.hpp>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <thread>
#include <vector>

#endif