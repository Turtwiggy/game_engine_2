#include "pch.hpp" // IWYU pragma: keep

#include "game_and_engine_interop.hpp"
#include "raws_components.hpp"

#include "modules/box2d/box2d_components.hpp"
#include "modules/box2d/box2d_helpers.hpp"
#include "modules/physics/physics_components.hpp"

namespace game2d {

SpriteComponent
default_sprite()
{
  const SpriteComponent s{
    .sprite_max_x = 32.0f,
    .sprite_max_y = 32.0f,
    .sprite_pos_x = 0.0f,
    .sprite_pos_y = 0.0f,
    .sprite_wh_x = 1.0f,
    .sprite_wh_y = 1.0f,
  };
  return s;
}

entt::entity
spawn(entt::registry& r, const SpawnConfig& data)
{
  const vec2 pos = data.pos;
  const vec2 size = data.size;
  const ColourComponent colour = data.colour;
  const bool is_static = data.is_static;
  const bool is_sensor = data.is_sensor;
  const bool is_emitter = data.is_emitter;
  const bool is_occluder = data.is_occluder;

  const auto world_id = SINGLE_Physics::get().worldId;
  // SDL_Log("Spawning thing at: %0.2f %0.2f", pos.x, pos.y);

  b2Vec2 size_meters = pixels_to_meters(size);
  b2Polygon box = b2MakeBox(0.5f * size_meters.x, 0.5f * size_meters.y);

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = is_static ? b2_staticBody : b2_dynamicBody;
  bodyDef.position = b2Vec2{ pixels_to_meters(pos) };
  bodyDef.fixedRotation = true;
  b2BodyId body_id = b2CreateBody(world_id, &bodyDef);

  b2Body_SetLinearDamping(body_id, 5.0f);

  b2ShapeDef shapeDef = b2DefaultShapeDef();
  shapeDef.isSensor = is_sensor;
  shapeDef.enableContactEvents = true;
  shapeDef.enableSensorEvents = true;
  b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shapeDef, &box);

  TransformComponent t_c;
  auto pos_2d = meters_to_pixels({ bodyDef.position.x, bodyDef.position.y });
  auto size_2d = meters_to_pixels(size_meters);
  t_c.pos = vec3{ pos_2d.x, pos_2d.y, 0.0f };
  t_c.size = vec3{ size_2d.x, size_2d.y, 0.0f };

  entt::entity e = r.create();
  r.emplace<TransformComponent>(e, t_c);
  r.emplace<ColourComponent>(e, ColourComponent{ .r = colour.r, .g = colour.g, .b = colour.b });
  r.emplace<SpriteComponent>(e, default_sprite());
  r.emplace<LightComponent>(e, LightComponent{ .is_emitter = (float)is_emitter, .is_occluder = (float)is_occluder });
  r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ .id = body_id, .shape_ids = { shape_id } });
  set_entity_from_body_id(body_id, e);

  entt::entity shape_e = r.create();
  r.emplace<PhysicsShapeComponent>(shape_e, PhysicsShapeComponent{ .body_id = body_id, .shape_id = shape_id });
  set_entity_from_shape_id(shape_id, shape_e);

  return e;
}

} // namespace game2d