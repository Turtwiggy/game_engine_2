#pragma once

#include "core/singleton.hpp"

#include <entt/fwd.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game2d {

struct SINGLE_Events : public Singleton<SINGLE_Events>
{
  entt::dispatcher dispatcher;

  SINGLE_Events() = default;
};

} // namespace game2d