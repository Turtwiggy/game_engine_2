#pragma once

#include <entt/fwd.hpp>

namespace game2d {

const auto scale = [](const float x, const float min, const float max, const float a, const float b) -> float {
  return ((b - a) * (x - min)) / (max - min) + a;
};

const auto wrap = [](const float x, const float min, const float max) {
  const float range = max - min;
  return min + fmod(fmod(x - min, range) + range, range);
};

} // namespace game2d