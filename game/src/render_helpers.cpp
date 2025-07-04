#include "core/pch.hpp"

#include "render_helpers.hpp"

#include "core/box2d/box2d_components.hpp"
#include "core/box2d/box2d_helpers.hpp"
#include "core/common.hpp"

namespace game2d {

void
update_transforms_from_physics(entt::registry& r)
{
  const auto view = r.view<const PhysicsBodyComponent, TransformComponent>();
  for (const auto& [e, pb_c, t_c] : view.each()) {

    const auto id = pb_c.id;
    const auto b2_pos = b2Body_GetPosition(id);
    const auto shape_ids = get_shapes(id);

    b2AABB aabb = b2Shape_GetAABB(shape_ids[0]); // assume every body has a shape
    for (int i = 1; i < shape_ids.size(); i++)
      aabb = b2AABB_Union(aabb, b2Shape_GetAABB(shape_ids[i]));

    const float w = aabb.upperBound.x - aabb.lowerBound.x;
    const float h = aabb.upperBound.y - aabb.lowerBound.y;
    const vec2 pos_in_pixels = meters_to_pixels(b2_pos);
    const vec2 size_in_pixels = meters_to_pixels({ w, h });
    const vec2 pos_tl = pos_in_pixels - (0.5f * size_in_pixels);
    t_c.pos = pos_tl;
    t_c.size = size_in_pixels;
    t_c.rotation_radians = b2Rot_GetAngle(b2Body_GetRotation(id));
  }
}

} // namespace game2d