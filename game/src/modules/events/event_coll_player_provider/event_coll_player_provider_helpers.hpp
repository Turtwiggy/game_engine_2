#pragma once

#include "modules/box2d/box2d_components.hpp"

#include <entt/fwd.hpp>

namespace game2d {

struct RedWizardComponent
{
  bool placeholder = true;
};

void
handle_on_coll_enter__player_provider(entt::registry& r, const OnCollisionEnter& evt);

void
handle_on_coll_enter__player_receiver(entt::registry& r, const OnCollisionEnter& evt);

void
handle_on_coll_enter__check_for_gameover(entt::registry& r, const OnCollisionEnter& evt);

} // namespace game2d