#pragma once

#include "modules/maths/vec.hpp"
#include <entt/fwd.hpp>

namespace game2d {

enum class ScreenAnchor
{
  LEFT,
  CENTER,
  RIGHT,

  TOP_LEFT,
  TOP_CENTER,
  TOP_RIGHT,

  CENTER_LEFT,
  CENTER_CENTER,
  CENTER_RIGHT,

  BOTTOM_LEFT,
  BOTTOM_CENTER,
  BOTTOM_RIGHT
};

vec2
pivot(const vec2 screen_wh, const ScreenAnchor anc, const vec2 pos);

} // namespace game2d