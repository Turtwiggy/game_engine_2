#pragma once

#include <entt/fwd.hpp>

namespace game2d {

struct ContainerProviderComponent
{
  bool placeholder = true;
};

struct ContainerReceiverComponent
{
  bool placeholder = true;
};

struct Request_WantsToPickup
{
  entt::entity e;
};

} // namespace game2d