#include "game_and_engine_interop.hpp"

namespace game2d {

float
random(RandomState& rnd, const float M, const float MN)
{
  const float scaled = (rnd.rng() - rnd.rng.min()) / (float)(rnd.rng.max() - rnd.rng.min() + 1.f);
  return scaled * (MN - M) + M;
};

float
random_01(RandomState& rnd)
{
  return random(rnd, 0.0f, 1.0f);
};

} // namespace game2d