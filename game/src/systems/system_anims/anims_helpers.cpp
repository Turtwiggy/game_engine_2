#include "pch.hpp"

#include "anims_helpers.hpp"

namespace game2d {

const float PI = 3.141592653;

SpriteDir
vec_to_dir(float x, float y, SpriteDir prv)
{
  if (x == 0.0f && y == 0.0f)
    return prv;

  float angle = atan2f(x, y); // 0 = south, increases CW (E, N, W)
  if (angle < 0)
    angle += 2.0f * PI;

  // 8 directions, each 45 degrees = PI/4
  // Add PI/8 offset to center the sectors
  int index = (int)((angle + PI / 8.0f) / (PI / 4.0f)) % 8;
  return static_cast<SpriteDir>(index);
}

} // namespace game2d