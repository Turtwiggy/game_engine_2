#include "pch.hpp" // IWYU pragma: keep

#include "entt/entity/entity.hpp"

#include "game_and_engine_interop.hpp"
#include "raws_components.hpp"

#include "modules/box2d/box2d_components.hpp"
#include "modules/box2d/box2d_helpers.hpp"
#include "modules/physics/physics_components.hpp"
#include "systems/system_shoot/shoot_components.hpp"

namespace game2d {

SpriteComponent
default_spritesheet(int sprite_x, int sprite_y)
{
  const SpriteComponent s{
    .sprite_max_x = 512 / 16,
    .sprite_max_y = 512 / 16,
    .sprite_pos_x = sprite_x,
    .sprite_pos_y = sprite_y,
    .spritesheet_idx = base_spritesheet_idx,
  };
  return s;
};

entt::entity
attach_body(entt::registry& r, entt::entity e, const PhysicsBodyDef& def)
{
  const auto pos_meters = def.pos_meters;
  const auto size_meters = def.size_meters;
  const auto is_bullet = def.is_bullet;
  const auto is_static = def.is_static;
  const auto is_sensor = def.is_sensor;
  const auto lin_damping = def.linear_damping;
  const auto ang_damping = def.angular_damping;

  const auto world_id = SINGLE_Physics::get().worldId;

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = is_static ? b2_staticBody : b2_dynamicBody;
  bodyDef.position = pos_meters;
  // bodyDef.rotation = body_def.
  bodyDef.fixedRotation = true;
  bodyDef.isBullet = is_bullet;
  bodyDef.linearVelocity = b2Vec2_zero;
  bodyDef.linearDamping = lin_damping;
  bodyDef.angularDamping = ang_damping;
  bodyDef.userData = (void*)static_cast<uintptr_t>(entt::to_integral(e));
  b2BodyId body_id = b2CreateBody(world_id, &bodyDef);

  b2Circle circle;
  // circle.center = def.pos_meters;
  circle.center = b2Vec2{ 0, 0 };
  circle.radius = 0.5f * size_meters.x;
  // b2Polygon box = b2MakeBox(0.5f * size_meters.x, 0.5f * size_meters.y);

  b2ShapeDef shapeDef = b2DefaultShapeDef();
  // shapeDef.density =
  // shapeDef.filter =
  // shapeDef.userData =
  shapeDef.isSensor = is_sensor;
  shapeDef.enableContactEvents = true;
  shapeDef.enableSensorEvents = true;

  b2ShapeId shape_id = b2CreateCircleShape(body_id, &shapeDef, &circle);
  // b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shapeDef, &box);

  r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ .id = body_id, .shape_ids = { shape_id } });
  set_entity_from_body_id(body_id, e);

  entt::entity shape_e = r.create();
  r.emplace<PhysicsShapeComponent>(shape_e, PhysicsShapeComponent{ .body_id = body_id, .shape_id = shape_id });
  set_entity_from_shape_id(shape_id, shape_e);

  return e;
};

void
attach_sprite(entt::registry& r, entt::entity e, const SpriteDef& def)
{
  const auto pos = def.pos;
  const auto size = def.size;
  const auto col = def.colour;
  const auto sprite = def.sprite;
  const auto is_emitter = def.is_emitter;
  const auto is_occluder = def.is_occluder;

  TransformComponent t_c;
  t_c.pos = vec3{ pos.x, pos.y, 0.0f };
  t_c.size = vec3{ size.x, size.y, 0.0f };
  r.emplace<TransformComponent>(e, t_c);
  r.emplace<ColourComponent>(e, ColourComponent{ .r = col.r, .g = col.g, .b = col.b });
  r.emplace<SpriteComponent>(e, sprite);
  r.emplace<LightComponent>(e, LightComponent{ .is_emitter = (float)is_emitter, .is_occluder = (float)is_occluder });
};

entt::entity
spawn_projectile(entt::registry& r,
                 const entt::entity weapon_e,
                 const BulletDef& def,
                 const vec2& pos,
                 const vec2& size,
                 const vec2& vel)
{
  auto e = r.create();

  SpriteComponent sprite_c = default_spritesheet();
  attach_sprite(r,
                e,
                SpriteDef{
                  .pos = pos,
                  .size = size,
                  .sprite = sprite_c,
                });

  attach_body(r,
              e,
              PhysicsBodyDef{
                .pos_meters = pixels_to_meters({ pos.x, pos.y }),
                .size_meters = pixels_to_meters({ size.x, size.y }),
                .is_bullet = true,
                .is_sensor = true,
              });

  // should attach bulletcomponent to the shape
  r.emplace<BulletComponent>(e, BulletComponent{ .weapon_e = weapon_e });

  // r.emplace<TeamComponent>(bullet_e, bullet_def.team);
  // r.emplace<SetTransformRotationBasedOnPhysicsVelocity>(bullet_e);
  // r.emplace<BulletBounce>(bullet_e, BulletBounce{ bullet_def.bounces });
  // r.emplace<BulletDamage>(bullet_e, bullet_def.damage);
  // r.emplace<BulletPierce>(bullet_e, bullet_def.pierce);
  // r.emplace<BulletSize>(bullet_e, bullet_def.size);
  // r.emplace<BulletSpeed>(bullet_e, bullet_def.speed);
  // r.emplace<BulletKnockback>(bullet_e, bullet_def.knockback_force);
  // r.emplace<BulletLifesteal>(bullet_e, bullet_def.lifesteal);
  // auto cc = bullet_def.crit_chance;
  // auto cd = bullet_def.crit_damage;
  // r.emplace<BulletCrit>(bullet_e, BulletCrit{ .crit_chance = cc, .crit_damage = cd });
  // r.emplace<BulletLifetime>(bullet_e, BulletLifetime{ .seconds = bullet_def.lifecycle / 1000.0f });
  // r.emplace<EntityTimedLifecycle>(bullet_e, bullet_def.lifecycle);

  const auto body_id = r.get<PhysicsBodyComponent>(e).id;
  b2Body_SetLinearVelocity(body_id, { vel.x, vel.y });

  return e;
}

} // namespace game2d