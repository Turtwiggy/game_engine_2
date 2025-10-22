#include "pch.hpp"

#include "camera_helpers.hpp"

namespace game2d {

vec2
worldspace_to_screenspace(const vec2 camera_pos, const vec2 pos, const vec2 screen_size)
{
  return (pos - camera_pos);
};

// vec3
// worldspace_to_screenspace(const vec3 camera_pos, const vec3 pos, const vec3 screen_size)
// {
//   return { 0, 0, 0 }; // todo
// };

} // namespace game2d