#pragma once

#include "singleton.hpp"

#include <entt/fwd.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game2d {

struct SINGLE_Events : public Singleton<SINGLE_Events>
{
  entt::dispatcher dispatcher;
};

} // namespace game2d