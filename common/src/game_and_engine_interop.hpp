#pragma once

#include "modules/maths/vec.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
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
  int sprite_max_x = 0;
  int sprite_max_y = 0;
  int sprite_pos_x = 0;
  int sprite_pos_y = 0;
  int sprite_wh_x = 1;
  int sprite_wh_y = 1;
  int spritesheet_idx = 0;
};

struct LightComponent
{
  float is_emitter = 0.0f;
  float is_occluder = 0.0f;
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

// Returns [0, 1)
float
random_01(RandomState& rnd);

struct Renderable
{
  TransformComponent transform;
  ColourComponent colour;
  SpriteComponent sprite;
  LightComponent light;
};

typedef struct Light
{
  float pos_x, pos_y, pos_z;
  float enabled;
} Light;

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

  std::vector<vec2> debug_inputs;

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
  SDL_GPUDevice* device = nullptr;

  entt::registry* r = nullptr;
  float dt = 0.0f;
  Uint64 dt_ns = 0;
  vec2 mouse_pos{ 0.0f, 0.0f };
  vec2 mouse_dt{ 0.0f, 0.0f };
  std::vector<SDL_Event> events;

  glm::vec3 camera_pos;

  CommonUiData ui_data;

  int n_preused_textures = 1; // custom texture
  std::vector<SDL_GPUTexture*> unprocessed_textures;
};

// intermediate data gamethread <=> renderthread
// gamethread will lock a one of the double-buffered renderdata,
// then write and update the data
// renderthread will then copy data it needs from this struct
struct RenderData
{
  std::mutex mtx; // mutex to protect access to data

  glm::vec3 camera_pos;

  // data
  std::vector<Renderable> renderable;
  std::vector<Light> lights;
  CommonUiData ui_data;

  std::vector<SDL_GPUTexture*> intermediate_textures;
};

// data owned by the RenderThread
struct GameUIData
{
  ImGuiContext* ctx;

  glm::vec3 camera_pos;

  std::vector<Renderable> renderable;
  std::vector<Light> lights;
  CommonUiData ui_data;

  int textures_to_free = 0;
  std::vector<SDL_GPUTexture*> renderthread_owned_textures;
};

} // namespace game2d