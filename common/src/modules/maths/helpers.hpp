#pragma once

#include <entt/fwd.hpp>

namespace game2d {

const auto scale = [](const float x, const float min, const float max, const float a, const float b) -> float {
  return ((b - a) * (x - min)) / (max - min) + a;
};

} // namespace game2d