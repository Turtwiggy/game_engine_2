#include "pch.hpp" // IWYU pragma: keep

#include "ui_helpers.hpp"

namespace game2d {

vec2
pivot(const vec2 screen_wh, const ScreenAnchor anc, const vec2 pos)
{
  if (anc == ScreenAnchor::BOTTOM_LEFT)
    return { pos.x, screen_wh.y - pos.y };

  return { 0.0f, 0.0f };
}

} // namespace game2d