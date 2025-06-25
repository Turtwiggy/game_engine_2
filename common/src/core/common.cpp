#include "common.hpp"

namespace game2d {

float
random(RandomState& rnd, const float M, const float MN)
{
  const float scaled = (rnd.rng() - rnd.rng.min()) / (float)(rnd.rng.max() - rnd.rng.min() + 1.f);
  return scaled * (MN - M) + M;
};

} // namespace game2d