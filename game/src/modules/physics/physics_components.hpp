#pragma once

#include <singleton.hpp>

#include <box2d/id.h>

namespace game2d {

struct SINGLE_Physics : public Singleton<SINGLE_Physics>
{
  b2WorldId worldId = b2_nullWorldId;
};

} // namespace game2d