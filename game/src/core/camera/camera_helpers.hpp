#pragma once

#include "core/common.hpp"

namespace game2d {

vec2
worldspace_to_screenspace(const vec2 camera_pos, const vec2 pos, const vec2 screen_size);

} // namespace game2d