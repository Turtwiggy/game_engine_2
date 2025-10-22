#pragma once

#include <entt/fwd.hpp>

namespace game2d {

// Attach this to entities to keep them between scenes
struct PersistentComponent
{
  bool placeholder = true;
};

} // namespace game2d