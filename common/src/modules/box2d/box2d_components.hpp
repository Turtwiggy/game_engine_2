#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>

namespace game2d {

struct PhysicsBodyComponent
{
  b2BodyId id = B2_ZERO_INIT;
  std::vector<b2ShapeId> shape_ids;
};

struct PhysicsShapeComponent
{
  b2BodyId body_id = B2_ZERO_INIT;
  b2ShapeId shape_id = B2_ZERO_INIT;
};

//
// events
//

struct OnCollisionEnter
{
  entt::entity shape_a = entt::null;
  entt::entity shape_b = entt::null;
};

struct OnCollisionExit
{
  entt::entity shape_a = entt::null;
  entt::entity shape_b = entt::null;
};

template<class A, class B>
std::pair<entt::entity, entt::entity>
coll(entt::registry& r, entt::entity a, entt::entity b)
{
  {
    const auto a_has_type_a = r.all_of<A>(a);
    const auto b_has_type_b = r.all_of<B>(b);
    if (a_has_type_a && b_has_type_b)
      return { a, b };
  }
  {
    const auto a_has_type_b = r.all_of<B>(a);
    const auto b_has_type_a = r.all_of<A>(b);
    if (a_has_type_b && b_has_type_a)
      return { b, a };
  }
  return { entt::null, entt::null };
};

} // namespace game2d