#pragma once

#include "box2d/box2d.h"

namespace game2d {

struct vec2
{
  float x = 0.0;
  float y = 0.0;

  vec2 operator+=(const vec2& other) const;
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

struct vec3
{
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

} // namespace game2d