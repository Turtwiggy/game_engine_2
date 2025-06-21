#include "common.hpp"

namespace game2d {

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
operator*(const float other, const vec2& v)
{
  return { other * v.x, other * v.y };
};

vec2
vec2::operator/(const vec2& other) const
{
  return { x / other.x, y / other.y };
};

constexpr float PIXELS_PER_METER = 50;

float
meters_to_pixels(float meters)
{
  return meters * PIXELS_PER_METER;
};
vec2
meters_to_pixels(b2Vec2 meters)
{
  return { meters.x * PIXELS_PER_METER, meters.y * PIXELS_PER_METER };
};
float
pixels_to_meters(float pixels)
{
  return pixels / PIXELS_PER_METER;
};
b2Vec2
pixels_to_meters(vec2 pixels)
{
  auto p = pixels / vec2(PIXELS_PER_METER, PIXELS_PER_METER);
  return b2Vec2{ p.x, p.y };
};

float
random(RandomState& rnd, const float M, const float MN)
{
  const float scaled = (rnd.rng() - rnd.rng.min()) / (float)(rnd.rng.max() - rnd.rng.min() + 1.f);
  return scaled * (MN - M) + M;
};

} // namespace game2d