#pragma once

#include "box2d/id.h"

namespace game2d {

b2WorldId
emplace_or_replace_physics_world();

void
physics_reset_task_count();

} // namespace game2d