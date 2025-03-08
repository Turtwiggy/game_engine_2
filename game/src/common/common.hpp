#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <imgui.h>

#include <random>

namespace game2d {

//
// some sort of shared file with the engine.
//

struct vec2
{
  float x = 0.0;
  float y = 0.0;

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

struct vec3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct PhysicsBodyComponent
{
  b2BodyId id;
};

float
meters_to_pixels(float meters);
vec2
meters_to_pixels(b2Vec2 meters);
float
pixels_to_meters(float pixels);
b2Vec2
pixels_to_meters(vec2 pixels);

struct TransformComponent
{
  vec2 pos{ 0, 0 };
  vec2 size{ 10, 10 };
  float rotation_radians = 0.0f;
};

struct RandomState
{
  std::minstd_rand rng;

  RandomState() = default;
  RandomState(const int seed) { rng.seed(seed); }
};

float
random(RandomState& rnd, const float M, const float MN);

struct GameData
{
  entt::registry& r;
  b2WorldId world_id;
  // vec2& mouse_pos;
};

struct GameUIData
{
  ImGuiContext* ctx;
};

} // namespace game2d