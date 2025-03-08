#include "render_helpers.hpp"

#include "box2d/box2d.h"
#include "common.hpp"

namespace game2d {

void
update_transforms_from_physics(entt::registry& r)
{
  const auto view = r.view<const PhysicsBodyComponent, TransformComponent>();
  for (const auto& [e, pb_c, t_c] : view.each()) {
    const auto b2_pos = b2Body_GetPosition(pb_c.id);
    const int n_shapes = b2Body_GetShapeCount(pb_c.id);
    std::vector<b2ShapeId> ids(n_shapes);
    b2Body_GetShapes(pb_c.id, ids.data(), (int)ids.size());

    if (ids.size() == 0)
      continue; // hmm.

    b2AABB aabb = b2Shape_GetAABB(ids[0]); // assume every body has a shape
    for (int i = 1; i < n_shapes; i++)
      aabb = b2AABB_Union(aabb, b2Shape_GetAABB(ids[i]));

    const float w = aabb.upperBound.x - aabb.lowerBound.x;
    const float h = aabb.upperBound.y - aabb.lowerBound.y;
    const vec2 pos_in_pixels = meters_to_pixels(b2_pos);
    const vec2 size_in_pixels = meters_to_pixels({ w, h });
    const vec2 pos_tl = pos_in_pixels - (0.5f * size_in_pixels);
    t_c.pos = pos_tl;
    t_c.size = size_in_pixels;
    t_c.rotation_radians = b2Rot_GetAngle(b2Body_GetRotation(pb_c.id));
  }
}

} // namespace game2d