#pragma once

#include "core/vec.hpp"

#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <imgui.h>

#include <mutex>
#include <random>

namespace game2d {

// some sort of shared file with the engine.
//

struct TransformComponent
{
  vec2 pos{ 0, 0 };
  vec2 size{ 10, 10 };
  float rotation_radians = 0.0f;
};

struct ColourComponent
{
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

//
// game components
//

struct RandomState
{
  std::minstd_rand rng;

  RandomState() = default;
  RandomState(const int seed) { rng.seed(seed); }
};

// note: not inclusive, so random(0, 4); will not include 4
float
random(RandomState& rnd, const float M, const float MN);

struct Renderable
{
  TransformComponent transform;
  ColourComponent colour;
};

struct InventoryComponent
{
  // std::vector<entt::entity> items;
  int items = 0;
};

// not sure about "UIEntity"
// this basically completely undoes the
// ecs architecture per-ui element
struct UIEntity
{
  entt::entity entity;
  Renderable renderable; // could be pointer?
  InventoryComponent inventory;
};

struct CommonUiData
{
  // data to show in UI
  float game_dt = 0.0f;
  int n_controllers = 0;

  vec2 keyboard_l{ 0, 0 };
  vec2 keyboard_r{ 0, 0 };
  vec2 controller_l{ 0, 0 };
  vec2 controller_r{ 0, 0 };

  int n_contact_events = 0;
  int n_sensor_events = 0;

  std::vector<UIEntity> hmm;
  // std::vector<std::pair<std::string, std::string>> something;
};

// data owned by the GameThread
struct GameData
{
  entt::registry* r = nullptr;
  float dt = 0.0f;
  b2WorldId world_id;

  vec2 camera_pos{ 0, 0 };
  vec2 mouse_pos{ 0, 0 };
  std::vector<SDL_Event> events;

  CommonUiData ui_data{};
};

// intermediate data gamethread <=> renderthread
// gamethread will lock a one of the double-buffered renderdata,
// then write and update the data
// renderthread will then copy data it needs from this struct
struct RenderData
{
  std::mutex mtx; // mutex to protect access to data

  // data
  std::vector<Renderable> renderable;
  vec2 camera_pos{ 0, 0 };

  CommonUiData ui_data;
};

// data owned by the RenderThread
struct GameUIData
{
  // data
  ImGuiContext* ctx;

  vec2 camera_pos{ 0, 0 };

  CommonUiData ui_data;

  std::vector<Renderable> renderable;
};

} // namespace game2d