#pragma once

#include <box2d/id.h>
#include <entt/fwd.hpp>

namespace game2d {

std::vector<b2ShapeId>
get_shapes(b2BodyId id);

void
update_transforms_from_physics(entt::registry& r);

} // namespace game2d