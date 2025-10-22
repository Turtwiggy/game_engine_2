#pragma once

#include "modules/maths/vec.hpp"

namespace game2d {

vec2
worldspace_to_screenspace(const vec2 camera_pos, const vec2 pos, const vec2 screen_size);

// vec3
// worldspace_to_screenspace(const vec3 camera_pos, const vec3 pos, const vec3 screen_size);

} // namespace game2d