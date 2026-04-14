#include "pch.hpp"

#include "event_coll_log_helpers.hpp"

#include "modules/box2d/box2d_helpers.hpp"

namespace game2d {

void
handle_on_coll_enter__log(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);

  SDL_Log("collision enter. s_eid: %i par_eid: %i, s_eid: %i, par_eid: %i ",
          (uint32_t)evt.shape_a,
          parent_a_e,
          (uint32_t)evt.shape_b,
          parent_b_e);
}

void
handle_on_coll_exit__log(entt::registry& r, const OnCollisionExit& evt)
{
  SDL_Log("collision exit.");
}

} // namespace game2d