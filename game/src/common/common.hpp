#pragma once

#include "dll.hpp"

#include "box2d/id.h"
#include <SDL3/SDL.h>
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <imgui.h>

#include <random>

namespace game2d {

// some sort of shared file with the engine.
//

struct DLL_API vec2
{
  float x = 0.0;
  float y = 0.0;

  vec2 operator+(const vec2& other) const;
  vec2 operator-(const vec2& other) const;
  vec2 operator*(const vec2& other) const;
  vec2 operator/(const vec2& other) const;

  vec2() = default;
  vec2(float x, float y)
    : x(x)
    , y(y) {};
  vec2(const b2Vec2& v)
    : x(v.x)
    , y(v.y) {};
  // operator b2Vec2() const { return b2Vec2(x, y); }
};

vec2
operator*(const float other, const vec2& v);

struct DLL_API vec3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct DLL_API PhysicsBodyComponent
{
  b2BodyId id = B2_ZERO_INIT;
};

struct DLL_API CameraComponent
{
  bool placeholder = true;
};

struct DLL_API PersistentComponent
{
  bool placeholder = true;
};

float
meters_to_pixels(float meters);
vec2
meters_to_pixels(b2Vec2 meters);
float
pixels_to_meters(float pixels);
b2Vec2
pixels_to_meters(vec2 pixels);

struct DLL_API TransformComponent
{
  vec2 pos{ 0, 0 };
  vec2 size{ 10, 10 };
  float rotation_radians = 0.0f;
};

struct DLL_API ColourComponent
{
  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;
};

struct DLL_API RandomState
{
  std::minstd_rand rng;

  RandomState() = default;
  RandomState(const int seed) { rng.seed(seed); }
};

float
random(RandomState& rnd, const float M, const float MN);

struct DLL_API CommonUiData
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

  // std::vector<std::pair<std::string, std::string>> something;
};

struct DLL_API GameData
{
  entt::registry* r = nullptr;

  b2WorldId world_id;

  vec2 mouse_pos{ 0, 0 };
  std::vector<SDL_Event> events;

  CommonUiData ui_data = {};
};

struct DLL_API GameUIData
{
  // data
  ImGuiContext* ctx;

  CommonUiData ui_data = {};
};

} // namespace game2d