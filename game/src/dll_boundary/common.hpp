#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>

//
// some sort of shared file with the engine.
//

struct vec2
{
  float x = 0.0;
  float y = 0.0;
};

struct vec3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

struct TransformComponent
{
  vec3 pos{ 0, 0 };
  vec2 size{ 10, 10 };
};

struct GameData
{
  entt::registry& r;
  // b2WorldId& world_id;
};