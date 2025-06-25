#include "core/pch.hpp"

#include "camera_helpers.hpp"

namespace game2d {

vec2
worldspace_to_screenspace(const vec2 camera_pos, const vec2 pos, const vec2 screen_size)
{
  return (pos - camera_pos);
};

} // namespace game2d