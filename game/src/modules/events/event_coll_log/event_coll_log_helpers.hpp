#pragma once

#include "modules/box2d/box2d_components.hpp"

#include <entt/fwd.hpp>

namespace game2d {

void
handle_on_coll_enter__log(entt::registry& r, const OnCollisionEnter& evt);

void
handle_on_coll_exit__log(entt::registry& r, const OnCollisionExit& evt);

} // namespace game2d