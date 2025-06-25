#include "core/pch.hpp"

#include "core/vec.hpp"

namespace game2d {

vec2
vec2::operator+=(const vec2& other) const
{
  return { x + other.x, y + other.y };
};

vec2
vec2::operator+(const vec2& other) const
{
  return { x + other.x, y + other.y };
};

vec2
vec2::operator-(const vec2& other) const
{
  return { x - other.x, y - other.y };
};

vec2
vec2::operator*(const vec2& other) const
{
  return { x * other.x, y * other.y };
};

vec2
vec2::operator/(const vec2& other) const
{
  return { x / other.x, y / other.y };
};

vec2
operator*(const float other, const vec2& v)
{
  return { other * v.x, other * v.y };
};

} // namespace game2d