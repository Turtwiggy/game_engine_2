#pragma once

#include "modules/camera/perspective_components.hpp"
#include "modules/maths/vec.hpp"

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
  vec3 pos{ 0, 0, 0 };
  vec3 size{ 10, 10, 10 };
  float rotation_radians = 0.0f;
};

struct ColourComponent
{
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct SpriteComponent
{
  float sprite_max_x = 0.0f;
  float sprite_max_y = 0.0f;
  float sprite_pos_x = 0.0f;
  float sprite_pos_y = 0.0f;
  float sprite_wh_x = 0.0f;
  float sprite_wh_y = 0.0f;
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
  SpriteComponent sprite;
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

  // set to true/false by game thread
  bool game_over = false;

  // set to true by ui thread, set to false by game thread.
  bool play_again = false;

  // std::vector<std::pair<std::string, std::string>> something;
};

// data owned by the GameThread
struct GameData
{
  entt::registry* r = nullptr;
  float dt = 0.0f;
  b2WorldId world_id;

  vec2 mouse_pos{ 0, 0 };
  vec2 mouse_dt{ 0, 0 };
  std::vector<SDL_Event> events;

  PerspectiveCamera camera_c;
  glm::vec3 camera_pos;

  CommonUiData ui_data;
};

// intermediate data gamethread <=> renderthread
// gamethread will lock a one of the double-buffered renderdata,
// then write and update the data
// renderthread will then copy data it needs from this struct
struct RenderData
{
  std::mutex mtx; // mutex to protect access to data

  PerspectiveCamera camera_c;
  glm::vec3 camera_pos;

  // data
  std::vector<Renderable> renderable;
  CommonUiData ui_data;
};

// data owned by the RenderThread
struct GameUIData
{
  ImGuiContext* ctx;

  PerspectiveCamera camera_c;
  glm::vec3 camera_pos;

  std::vector<Renderable> renderable;
  CommonUiData ui_data;
};

} // namespace game2d