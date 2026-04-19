#include "pch.hpp"

#include "physics_system.hpp"

#include "box2d_parallel.hpp"
#include "modules/box2d/box2d_components.hpp"
#include "modules/events/events_core/events_components.hpp"
#include "physics_components.hpp"

namespace game2d {

void
update_physics_system(GameData* data, entt::registry& r, const float dt)
{
  const auto world_id = SINGLE_Physics::get().worldId;
  auto& events_c = SINGLE_Events::get();

  // update world
  {
    const int substep_count = 4;
    b2World_Step(world_id, dt, substep_count);
    physics_reset_task_count();
  }

  // Generate contact events
  {
    const auto convert_box2d_coll_to_entt = [](entt::registry& r, const b2ShapeId a, const b2ShapeId b, auto callback) {
      const auto shape_eid_a = (entt::entity)(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(a)));
      const auto shape_eid_b = (entt::entity)(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(b)));

      assert(shape_eid_a != entt::null);
      assert(shape_eid_a != entt::null);
      callback(shape_eid_a, shape_eid_b);
    };

    const b2ContactEvents c_events = b2World_GetContactEvents(world_id);
    const b2SensorEvents s_events = b2World_GetSensorEvents(world_id);

    auto& ui_data = data->ui_data;
    ui_data.n_sensor_events = s_events.beginCount + s_events.endCount;
    ui_data.n_contact_events = c_events.beginCount + c_events.endCount;

    for (int i = 0; i < c_events.beginCount; ++i) {
      b2ContactBeginTouchEvent* beginEvent = c_events.beginEvents + i;
      const auto callback = [&events_c](const entt::entity e_a, const entt::entity e_b) {
        events_c.dispatcher.trigger(OnCollisionEnter{ .shape_a = e_a, .shape_b = e_b });
      };
      convert_box2d_coll_to_entt(r, beginEvent->shapeIdA, beginEvent->shapeIdB, callback);
    }
    for (int i = 0; i < c_events.endCount; ++i) {
      b2ContactEndTouchEvent* endEvent = c_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->shapeIdA) && b2Shape_IsValid(endEvent->shapeIdB)) {
        const auto callback = [&events_c](const entt::entity e_a, const entt::entity e_b) {
          events_c.dispatcher.trigger(OnCollisionExit{ .shape_a = e_a, .shape_b = e_b });
        };
        convert_box2d_coll_to_entt(r, endEvent->shapeIdA, endEvent->shapeIdB, callback);
      }
    }
    for (int i = 0; i < s_events.beginCount; ++i) {
      b2SensorBeginTouchEvent* beginEvent = s_events.beginEvents + i;
      const auto callback = [&events_c](const entt::entity e_a, const entt::entity e_b) {
        events_c.dispatcher.trigger(OnCollisionEnter{ .shape_a = e_a, .shape_b = e_b });
      };
      convert_box2d_coll_to_entt(r, beginEvent->sensorShapeId, beginEvent->visitorShapeId, callback);
    }
    for (int i = 0; i < s_events.endCount; ++i) {
      b2SensorEndTouchEvent* endEvent = s_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->sensorShapeId) && b2Shape_IsValid(endEvent->visitorShapeId)) {
        const auto callback = [&events_c](const auto e_a, const auto e_b) {
          events_c.dispatcher.trigger(OnCollisionExit{ .shape_a = e_a, .shape_b = e_b });
        };
        convert_box2d_coll_to_entt(r, endEvent->sensorShapeId, endEvent->visitorShapeId, callback);
      }
    }

    SINGLE_Events::get().dispatcher.update();
  }
}

} // namespace game2d