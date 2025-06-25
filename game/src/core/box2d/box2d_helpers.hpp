#pragma once

#include "core/common.hpp"

#include <box2d/box2d.h>
#include <entt/fwd.hpp>

namespace game2d {

void
set_entity_from_body_id(const b2BodyId id, const entt::entity eid);

void
set_entity_from_shape_id(const b2ShapeId id, const entt::entity eid);

entt::entity
get_entity_from_body_id(const b2BodyId id);

entt::entity
get_entity_from_shape_id(const b2ShapeId id);

entt::entity
get_parent_entity_from_shape_id(entt::registry& r, const b2ShapeId id);

std::vector<b2ShapeId>
get_shapes(b2BodyId id);

float
meters_to_pixels(float meters);
vec2
meters_to_pixels(b2Vec2 meters);
float
pixels_to_meters(float pixels);
b2Vec2
pixels_to_meters(vec2 pixels);

} // namespace game2d