#include "game.hpp"

#include "common.hpp"
#include "render_helpers.hpp"
#include "singleton.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_log.h>
#include <algorithm>
#include <box2d/box2d.h>
#include <entt/fwd.hpp>
#include <imgui.h>

#include <chrono>

namespace game2d {

static entt::registry internal_r;
static bool refreshed = false;

static vec2 keyboard_l{ 0, 0 };
static vec2 keyboard_r{ 0, 0 };
static vec2 controller_l{ 0, 0 };
static vec2 controller_r{ 0, 0 };
static vec2 l_input{ 0, 0 };
static vec2 r_input{ 0, 0 };
static bool jump = false;
const auto jump_force = b2Vec2{ 0.0f, -5.0f };
const auto move_force = 0.5f;
static b2Vec2 gravity = { 0.0f, 0.0f };

const auto get_system_time_for_seed = []() -> int {
  auto now = std::chrono::high_resolution_clock::now();
  long long seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  return seed;
};

struct OnCollisionEnter
{
  entt::entity a = entt::null;
  entt::entity b = entt::null;
};
struct OnCollisionExit
{
  entt::entity a = entt::null;
  entt::entity b = entt::null;
};

void
spawn(GameData* data,
      const vec2 pos,
      const vec2 size,
      const ColourComponent colour,
      const bool is_static = false,
      const bool is_sensor = false)
{
  auto& r = internal_r;

  b2Vec2 size_meters = pixels_to_meters(size);
  b2Polygon box = b2MakeBox(0.5f * size_meters.x, 0.5f * size_meters.y);

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = is_static ? b2_staticBody : b2_dynamicBody;
  bodyDef.position = b2Vec2{ pixels_to_meters(pos) };
  bodyDef.fixedRotation = true;
  b2BodyId body_id = b2CreateBody(data->world_id, &bodyDef);

  b2Body_SetLinearDamping(body_id, 5.0f);

  b2ShapeDef shapeDef = b2DefaultShapeDef();
  shapeDef.isSensor = is_sensor;
  shapeDef.enableContactEvents = true;
  shapeDef.enableSensorEvents = true;
  b2CreatePolygonShape(body_id, &shapeDef, &box);

  TransformComponent t_c;
  t_c.pos = meters_to_pixels({ bodyDef.position.x, bodyDef.position.y });
  t_c.size = meters_to_pixels(size_meters);
  entt::entity e = r.create();
  r.emplace<TransformComponent>(e, t_c);

  int seed = 0;
#if defined(_DEBUG)
  seed = get_system_time_for_seed();
#endif
  // static RandomState rnd(seed);
  // float rnd_r = random(rnd, 0.0f, 1.0f);
  // float rnd_g = random(rnd, 0.0f, 1.0f);
  // float rnd_b = random(rnd, 0.0f, 1.0f);
  r.emplace<ColourComponent>(e, ColourComponent{ .r = colour.r, .g = colour.g, .b = colour.b });

  r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ body_id });
};

void
handle_on_coll_enter__log(entt::registry& r, const OnCollisionEnter& coll_evt)
{
  SDL_Log("collision enter.");
}

void
handle_on_coll_exit__log(entt::registry& r, const OnCollisionExit& coll_evt)
{
  SDL_Log("collision exit.");
}

void
game_init(GameData* data)
{
  SDL_Log("(GameEngine) Init()");

  // sets as an instance of an entt::registry used by this dll
  data->r = &internal_r;
  auto& r = internal_r;

  {
    const auto& view = r.view<TransformComponent, ColourComponent>();
    SDL_Log("renderables: %zu", view.size_hint());
  }

  b2WorldDef world_def = b2DefaultWorldDef();
  // world_def.workerCount = worker_count;
  // world_def.enqueueTask = EnqueueTask;
  // world_def.finishTask = FinishTask;
  // world_def.userTaskContext = s.get();
  world_def.gravity = gravity;
  world_def.enableSleep = true;
  data->world_id = b2CreateWorld(&world_def);

  spawn(data, { 300, 300 }, { 50, 50 }, { 1.0f, 0.0f, 0.0f });
  spawn(data, { 900, 450 }, { 50, 50 }, { 0.0f, 1.0f, 0.0f });
  // spawn(data, { 1280 * 0.5f, 720 * 0.75f }, { 1000, 50 }, true);

  spawn(data, { 450, 450 }, { 50, 50 }, { 0.0f, 0.0f, 1.0f }, false, true);

  // setup events
  static entt::dispatcher dispatcher;
  auto& evts_c = SINGLE_Events::get();
  evts_c.dispatcher = &dispatcher;
  evts_c.dispatcher->sink<OnCollisionEnter>().connect<&handle_on_coll_enter__log>(r);
  evts_c.dispatcher->sink<OnCollisionExit>().connect<&handle_on_coll_exit__log>(r);
};

void
game_fixed_update(GameData* data)
{
  auto& r = internal_r;

  // Apply force to first dynamic body
  {
    auto view = r.view<const PhysicsBodyComponent, const TransformComponent>();
    for (const auto& [e, pb_c, t_c] : view.each()) {
      const b2BodyType type = b2Body_GetType(pb_c.id);
      if (type == b2_staticBody)
        continue;

      // const auto meters_per_second = 0.1f;
      const auto force = move_force * b2Vec2{ l_input.x, l_input.y };
      b2Body_ApplyLinearImpulseToCenter(pb_c.id, force, true);

      break;
    }
  }

  // Apply jump force
  {
    entt::entity first_dynamic_e = entt::null;
    auto view = r.view<const PhysicsBodyComponent, const TransformComponent>();
    for (const auto& [e, pb_c, t_c] : view.each()) {
      const b2BodyType type = b2Body_GetType(pb_c.id);
      if (type == b2_staticBody)
        continue;
      first_dynamic_e = e;
      break;
    }
    if (first_dynamic_e != entt::null) {
      if (jump) {
        auto& pb_c = r.get<PhysicsBodyComponent>(first_dynamic_e);
        b2Body_ApplyLinearImpulseToCenter(pb_c.id, jump_force, true);
      }
      jump = false;
    }
  }

  // update world
  {
    constexpr int physics_substep_count = 4;
    constexpr float physics_dt = 1.0f / 60.0f;
    b2World_Step(data->world_id, physics_dt, physics_substep_count);
  }

  // Generate contact events.
  {
    const auto convert_box2d_coll_to_entt = [](entt::registry& r, const b2ShapeId a, const b2ShapeId b, auto callback) {
      const auto fixture_eid_a = static_cast<entt::entity>((reinterpret_cast<uintptr_t>(b2Shape_GetUserData(a))));
      const auto fixture_eid_b = static_cast<entt::entity>((reinterpret_cast<uintptr_t>(b2Shape_GetUserData(b))));
      callback(fixture_eid_a, fixture_eid_b);
    };
    const b2ContactEvents c_events = b2World_GetContactEvents(data->world_id);
    const b2SensorEvents s_events = b2World_GetSensorEvents(data->world_id);
    auto& ui_data = data->ui_data;
    ui_data.n_sensor_events = s_events.beginCount + s_events.endCount;
    ui_data.n_contact_events = c_events.beginCount + c_events.endCount;

    for (int i = 0; i < c_events.beginCount; ++i) {
      b2ContactBeginTouchEvent* beginEvent = c_events.beginEvents + i;
      convert_box2d_coll_to_entt(r, beginEvent->shapeIdA, beginEvent->shapeIdB, [&r](const auto e_a, const auto e_b) {
        SINGLE_Events::get().dispatcher->trigger(OnCollisionEnter{ .a = e_a, .b = e_b });
      });
    }
    for (int i = 0; i < c_events.endCount; ++i) {
      b2ContactEndTouchEvent* endEvent = c_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->shapeIdA) && b2Shape_IsValid(endEvent->shapeIdB)) {
        convert_box2d_coll_to_entt(r, endEvent->shapeIdA, endEvent->shapeIdB, [](const auto e_a, const auto e_b) {
          SINGLE_Events::get().dispatcher->trigger(OnCollisionExit{ .a = e_a, .b = e_b });
        });
      }
    }
    for (int i = 0; i < s_events.beginCount; ++i) {
      b2SensorBeginTouchEvent* beginEvent = s_events.beginEvents + i;
      convert_box2d_coll_to_entt(
        r, beginEvent->sensorShapeId, beginEvent->visitorShapeId, [&r](const auto e_a, const auto e_b) {
          SINGLE_Events::get().dispatcher->trigger(OnCollisionEnter{ .a = e_a, .b = e_b });
        });
    }
    for (int i = 0; i < s_events.endCount; ++i) {
      b2SensorEndTouchEvent* endEvent = s_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->sensorShapeId) && b2Shape_IsValid(endEvent->visitorShapeId)) {
        convert_box2d_coll_to_entt(r, endEvent->sensorShapeId, endEvent->visitorShapeId, [](const auto e_a, const auto e_b) {
          SINGLE_Events::get().dispatcher->trigger(OnCollisionExit{ .a = e_a, .b = e_b });
        });
      }
    }
    SINGLE_Events::get().dispatcher->update();
  }
};

void
game_update(GameData* data)
{
  const auto evts = data->events;
  auto& r = internal_r;

  // note: remove static at some point
  // static float timer_cur = 0.0f;
  // static float timer_max = 0.1f;
  // timer_cur += data->dt;
  // if (timer_cur >= timer_max) {
  //   SDL_Log("X seconds elapsed...");
  //   timer_cur -= timer_max;
  // }

  const auto scale = [](const float x, const float min, const float max, const float a, const float b) -> float {
    return ((b - a) * (x - min)) / (max - min) + a;
  };

  //
  // input events
  //

  for (const SDL_Event& evt : evts) {
    //
    // button events
    //
    if (evt.type == SDL_EVENT_KEY_DOWN) {
      const auto scancode = evt.key.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = evt.key.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyDown %s %i %i", scancode_name, down, repeat);

      if (scancode == SDL_SCANCODE_W)
        keyboard_l.y = -1;
      if (scancode == SDL_SCANCODE_S)
        keyboard_l.y = 1;
      if (scancode == SDL_SCANCODE_A)
        keyboard_l.x = -1;
      if (scancode == SDL_SCANCODE_D)
        keyboard_l.x = 1;
      if (scancode == SDL_SCANCODE_KP_0)
        spawn(data, data->mouse_pos, { 50, 50 }, { 1.0, 1.0, 1.0 });
      if (scancode == SDL_SCANCODE_SPACE)
        jump |= true;
    }
    if (evt.type == SDL_EVENT_KEY_UP) {
      const SDL_KeyboardEvent& k_evt = evt.key;
      const auto scancode = k_evt.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = k_evt.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyUp %s %i %i", scancode_name, down, repeat);

      if (scancode == SDL_SCANCODE_W)
        keyboard_l.y = 0;
      if (scancode == SDL_SCANCODE_S)
        keyboard_l.y = 0;
      if (scancode == SDL_SCANCODE_A)
        keyboard_l.x = 0;
      if (scancode == SDL_SCANCODE_D)
        keyboard_l.x = 0;
    }
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      SDL_MouseButtonEvent m_evt = evt.button;

      // test if you click a shape
      const auto mouse_pos = data->mouse_pos;
      auto view = r.view<const PhysicsBodyComponent>();
      for (const auto& [e, pb_c] : view.each()) {
        auto shape_ids = get_shapes(pb_c.id);
        if (b2Shape_TestPoint(shape_ids[0], pixels_to_meters(mouse_pos))) {

          // Destroy now!
          b2DestroyBody(pb_c.id);
          r.destroy(e);
        }
      }
    }
    //
    // joysticks
    //
    if (evt.type == SDL_EVENT_JOYSTICK_ADDED) {
      auto joystick_id = evt.jdevice.which;
      SDL_Log("Joystick Added: %i", joystick_id);

      // open as gamepad?
      if (SDL_IsGamepad(joystick_id)) {
        SDL_Log("tried to add as gamepad.");
        SDL_Gamepad* gamepad = SDL_OpenGamepad(joystick_id);
        if (gamepad)
          SDL_Log("gamepad: success?");
        else
          SDL_Log("gamepad: fail?");
      }
    }
    if (evt.type == SDL_EVENT_JOYSTICK_REMOVED) {
      auto joystick_id = evt.jdevice.which;
      SDL_Log("Joystick Removed: %i", joystick_id);
      // TODO: remove from some list or other
    }
    if (evt.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN) {
      SDL_Log("Joystick button down");
    }
    if (evt.type == SDL_EVENT_JOYSTICK_BUTTON_UP) {
      SDL_Log("Joystick button up");
    }
  }

  auto& ui_data = data->ui_data;
  ui_data.keyboard_l = keyboard_l;
  ui_data.keyboard_r = keyboard_r;

  int n_joysticks = 0;
  auto* joysticks = SDL_GetJoysticks(&n_joysticks);
  ui_data.n_controllers = n_joysticks;
  ui_data.controller_l = { 0, 0 };
  ui_data.controller_r = { 0, 0 };

  for (int i = 0; i < n_joysticks; i++) {
    const SDL_JoystickID id = joysticks[i];
    const auto instance = SDL_GetJoystickFromID(id);

    const auto lx_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_LEFTX);
    const auto ly_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_LEFTY);
    const auto rx_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_RIGHTX);
    const auto ry_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_RIGHTY);

    const auto lx_nrm = scale(lx_raw, -32768, 32767, -1.0f, 1.0f);
    const auto ly_nrm = scale(ly_raw, -32768, 32767, -1.0f, 1.0f);
    const auto rx_nrm = scale(rx_raw, -32768, 32767, -1.0f, 1.0f);
    const auto ry_nrm = scale(ry_raw, -32768, 32767, -1.0f, 1.0f);
    const vec2 inp_l = vec2{ lx_nrm, ly_nrm };
    const vec2 inp_r = vec2{ rx_nrm, ry_nrm };
    controller_l = inp_l;
    controller_r = inp_r;

    // generate inputs for one button.
    static bool s_press = false;
    static bool s_held_last_frame = false;
    static bool s_held = false;
    static bool s_release = false;
    s_press = false;
    s_release = false;
    s_held = SDL_GetJoystickButton(instance, SDL_GAMEPAD_BUTTON_SOUTH);
    if (s_held_last_frame && !s_held)
      s_release = true;
    if (!s_held_last_frame && s_held)
      s_press = true;
    s_held_last_frame = s_held;

    if (s_press)
      SDL_Log("south button pressed");
    jump |= s_press;

    ui_data.controller_l.x = lx_nrm;
    ui_data.controller_l.y = ly_nrm;
    ui_data.controller_r.x = rx_nrm;
    ui_data.controller_r.y = ry_nrm;
    break;
  }

  l_input = keyboard_l + controller_l;
  r_input = keyboard_r + controller_r;

  // clamp the inputs
  l_input.x = std::clamp(l_input.x, -1.0f, 1.0f);
  l_input.y = std::clamp(l_input.y, -1.0f, 1.0f);
  r_input.x = std::clamp(r_input.x, -1.0f, 1.0f);
  r_input.y = std::clamp(r_input.y, -1.0f, 1.0f);

  // Update transforms via physics body.
  update_transforms_from_physics(r);

  // update_events_system()
  auto& events_c = SINGLE_Events::get();
  events_c.dispatcher->update();
};

void
game_update_ui(GameUIData* ui_data)
{
  ImGui::SetCurrentContext(ui_data->ctx);
  const auto& data = ui_data->ui_data;

  auto flags = 0;
  flags |= ImGuiWindowFlags_NoDecoration;
  flags |= ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::Begin("SomeOtherCrazyWindow", nullptr, flags);
  ImGui::Text("(GameThread) FPS: %0.2f", 1.0f / data.game_dt);
  ImGui::Text("(RenderThread) FPS: %0.2f", ImGui::GetIO().Framerate);
  ImGui::Text("contact events: %i", data.n_contact_events);
  ImGui::Text("sensor events: %i", data.n_sensor_events);
  ImGui::End();

  // int controllers = 0;
  // SDL_GetJoysticks(&controllers);
  // ImGui::Text("n_controllers: %i", controllers);

  ImGui::Begin("SomeOtherWindow", nullptr, flags);

  ImGui::Text("Keyboard");
  ImGui::Text("%f %f %f %f", data.keyboard_l.x, data.keyboard_l.y, data.keyboard_r.x, data.keyboard_r.y);

  ImGui::Text("Controllers");

  ImGui::Text("%i %f %f %f %f",
              data.n_controllers,
              data.controller_l.x,
              data.controller_l.y,
              data.controller_r.x,
              data.controller_r.y);

  ImGui::Text("Input");
  ImGui::Text("%f %f %f %f", l_input.x, l_input.y, r_input.x, r_input.y);

  ImGui::End();
};

void
game_refresh(GameData* data)
{
  SDL_Log("(GameEngine) game_refresh()");
  refreshed = true;

  // Delete the physics world. Create another one.
  b2DestroyWorld(data->world_id);
  data->world_id = {};

  // deletes all the physicsbody
  // auto& r = *data->r;
  // auto view = r.view<PhysicsBodyComponent>();
  // view.each([&r](const auto e, const auto& pb_c) { r.destroy(e); });
};

} // namespace game2d