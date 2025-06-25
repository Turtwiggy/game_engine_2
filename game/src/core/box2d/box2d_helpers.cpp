#include "core/pch.hpp"

#include "core/box2d/box2d_helpers.hpp"

#include "core/box2d/box2d_components.hpp"

namespace game2d {

void
set_entity_from_body_id(const b2BodyId id, const entt::entity eid)
{
  b2Body_SetUserData(id, (void*)static_cast<uintptr_t>(entt::to_integral(eid)));
}

void
set_entity_from_shape_id(const b2ShapeId id, const entt::entity eid)
{
  b2Shape_SetUserData(id, (void*)static_cast<uintptr_t>(entt::to_integral(eid)));
}

entt::entity
get_entity_from_body_id(const b2BodyId id)
{
  return (entt::entity) reinterpret_cast<uintptr_t>(b2Body_GetUserData(id));
};

entt::entity
get_entity_from_shape_id(const b2ShapeId id)
{
  return (entt::entity) reinterpret_cast<uintptr_t>(b2Shape_GetUserData(id));
};

entt::entity
get_parent_entity_from_shape_id(entt::registry& r, const b2ShapeId id)
{
  const auto shape_e = get_entity_from_shape_id(id);
  const auto& shape_c = r.get<const PhysicsShapeComponent>(shape_e);
  return get_entity_from_body_id(shape_c.body_id);
};

std::vector<b2ShapeId>
get_shapes(b2BodyId id)
{
  const int n_shapes = b2Body_GetShapeCount(id);
  std::vector<b2ShapeId> ids(n_shapes);
  b2Body_GetShapes(id, ids.data(), (int)ids.size());
  return ids;
};

constexpr float PIXELS_PER_METER = 50;

float
meters_to_pixels(float meters)
{
  return meters * PIXELS_PER_METER;
};
vec2
meters_to_pixels(b2Vec2 meters)
{
  return { meters.x * PIXELS_PER_METER, meters.y * PIXELS_PER_METER };
};
float
pixels_to_meters(float pixels)
{
  return pixels / PIXELS_PER_METER;
};
b2Vec2
pixels_to_meters(vec2 pixels)
{
  auto p = pixels / vec2(PIXELS_PER_METER, PIXELS_PER_METER);
  return b2Vec2{ p.x, p.y };
};

} // namespace game2d