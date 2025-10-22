#include "pch.hpp"

#include "game.hpp"

#include "actors/actor_player/actor_player_components.hpp"
#include "game_and_engine_interop.hpp"
#include "modules/box2d/box2d_components.hpp"
#include "modules/box2d/box2d_helpers.hpp"
#include "modules/camera/camera_helpers.hpp"
#include "modules/camera/perspective_helpers.hpp"
#include "modules/entt/entt_helpers.hpp"
#include "modules/input/input_helpers.hpp"
#include "modules/maths/helpers.hpp"
#include "render_helpers.hpp"
#include "systems/system_events/events_components.hpp"
#include "systems/system_items/items_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_system.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>

namespace game2d {

static entt::registry internal_r;
static bool refreshed = false;
const auto screen_size = vec2(1280, 720); // todo: fix this

static bool capture_mouse = false;
static float camera_speed = 10.0f;
static float mouse_sens = 0.01f;

static float stuff_size = 32.0f;
// static vec2 keyboard_l{ 0, 0 };
// static vec2 keyboard_r{ 0, 0 };
static vec2 controller_l{ 0, 0 };
static vec2 controller_r{ 0, 0 };
// static vec2 l_input{ 0, 0 };
// static vec2 r_input{ 0, 0 };
static bool jump = false;
static bool pickup = false;
const auto jump_force = b2Vec2{ 0.0f, -5.0f };
const auto move_force = 0.25f;
static b2Vec2 gravity = { 0.0f, 0.0f };

const auto get_system_time_for_seed = []() -> int {
  auto now = std::chrono::high_resolution_clock::now();
  long long seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  return seed;
};

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
  b2ShapeId shape_id = b2CreatePolygonShape(body_id, &shapeDef, &box);

  TransformComponent t_c;
  auto pos_2d = meters_to_pixels({ bodyDef.position.x, bodyDef.position.y });
  auto size_2d = meters_to_pixels(size_meters);
  t_c.pos = vec3{ pos_2d.x, pos_2d.y, 0.0f };
  t_c.size = vec3{ size_2d.x, size_2d.y, 0.0f };

  // float rnd_r = random(rnd, 0.0f, 1.0f);
  // float rnd_g = random(rnd, 0.0f, 1.0f);
  // float rnd_b = random(rnd, 0.0f, 1.0f);

  entt::entity e = r.create();
  r.emplace<TransformComponent>(e, t_c);
  r.emplace<ColourComponent>(e, ColourComponent{ .r = colour.r, .g = colour.g, .b = colour.b });
  r.emplace<SpriteComponent>(e, default_sprite());
  r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ .id = body_id, .shape_ids = { shape_id } });
  set_entity_from_body_id(body_id, e);

  entt::entity shape_e = r.create();
  r.emplace<PhysicsShapeComponent>(shape_e, PhysicsShapeComponent{ .body_id = body_id, .shape_id = shape_id });
  set_entity_from_shape_id(shape_id, shape_e);

  return e;
};

void
handle_on_coll_enter__check_for_gameover(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);
  const auto [shape_a, shape_b] = coll<const PlayerComponent, const ContainerReceiverComponent>(r, parent_a_e, parent_b_e);

  if (shape_a == entt::null || shape_b == entt::null)
    return; // not a coll of interest

  const auto& consumer_inv = r.get<const InventoryComponent>(parent_b_e);
  SDL_Log("consumer has: %i items", consumer_inv.items);

  const bool gameover = consumer_inv.items >= 5;
  if (gameover) {
    SDL_Log("dingding! gameover");
    create_empty<Request_GameOver>(r);
  }
}

void
handle_on_coll_enter__log(entt::registry& r, const OnCollisionEnter& evt)
{
  const auto parent_a_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_a).body_id);
  const auto parent_b_e = get_entity_from_body_id(r.get<const PhysicsShapeComponent>(evt.shape_b).body_id);

  // SDL_Log("collision enter. s_eid: %i par_eid: %i, s_eid: %i, par_eid: %i ",
  //         (uint32_t)evt.shape_a,
  //         parent_a_e,
  //         (uint32_t)evt.shape_b,
  //         parent_b_e);

  {
    const auto [shape_a, shape_b] = coll<const PlayerComponent, const ContainerProviderComponent>(r, parent_a_e, parent_b_e);
    if (shape_a != entt::null && shape_b != entt::null) {
      SDL_Log("collision enter with provider.");

      auto& provider_inv = r.get<InventoryComponent>(parent_b_e);
      if (provider_inv.items <= 0)
        return; // no more items to give
      provider_inv.items--;

      auto& player_inv = r.get<InventoryComponent>(parent_a_e);
      player_inv.items++;
    }
  }

  {
    const auto [shape_a, shape_b] = coll<const PlayerComponent, const ContainerReceiverComponent>(r, parent_a_e, parent_b_e);
    if (shape_a != entt::null && shape_b != entt::null) {
      SDL_Log("collision enter with reciever.");

      auto& player_inv = r.get<InventoryComponent>(parent_a_e);
      if (player_inv.items <= 0)
        return; // no item on player
      player_inv.items--;

      auto& consumer_inv = r.get<InventoryComponent>(parent_b_e);
      consumer_inv.items++;

      // check for gameover
      handle_on_coll_enter__check_for_gameover(r, evt);
    }
  }
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

  auto& camera_pos = data->camera_pos;
  camera_pos = { -7.5, 3.2, -4.45 };

  auto& camera_c = data->camera_c;
  camera_c.projection = calculate_perspective_projection(screen_size.x, screen_size.y);
  camera_c.pitch = 0.24f;
  camera_c.yaw = 2.19;

  {
    const auto& view = r.view<const TransformComponent, const ColourComponent, const SpriteComponent>();
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

  // spawn(data, { 1280 * 0.5f, 720 * 0.75f }, { 1000, 50 }, true); // static

  // rnd_x on left side of screen.
  int seed = 0;
#if defined(_DEBUG)
  seed = get_system_time_for_seed();
#endif
  static RandomState rnd(seed);
  const auto rnd_0_x = random(rnd, 100.0f, 450.0f);

  for (int i = 0; i < 100; i++) {
    const auto rnd_x = random(rnd, 550.0f, 900.0f);
    const auto rnd_y = random(rnd, 0.0f, 720.0f);
    const auto consumer_e = spawn(data, { rnd_x, rnd_y }, { stuff_size, stuff_size }, { 0.0f, 1.0f, 0.0f });
    r.emplace<ContainerReceiverComponent>(consumer_e);
    r.emplace<InventoryComponent>(consumer_e, InventoryComponent{ .items = 0 });
  }

  const auto provider_e = spawn(data, { rnd_0_x, 300 }, { stuff_size, stuff_size }, { 1.0f, 0.0f, 0.0f });
  r.emplace<ContainerProviderComponent>(provider_e);
  r.emplace<InventoryComponent>(provider_e, InventoryComponent{ .items = 5 });

  const auto player_e = spawn(data, { 500, 450 }, { stuff_size, stuff_size }, { 0.0f, 1.0f, 1.0f }, false, true);
  r.emplace<PlayerComponent>(player_e);
  r.emplace<InventoryComponent>(player_e, InventoryComponent{ .items = 0 });

  // setup events
  auto& evts_c = SINGLE_Events::get();
  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__log>(r);
  evts_c.dispatcher.sink<OnCollisionExit>().connect<&handle_on_coll_exit__log>(r);
  evts_c.dispatcher.sink<OnCollisionEnter>().connect<&handle_on_coll_enter__check_for_gameover>(r);
};

void
game_fixed_update(GameData* data)
{
  auto& r = internal_r;

  // Apply force to first dynamic body
  /*
  {
    auto view = r.view<const PhysicsBodyComponent, const TransformComponent, const PlayerComponent>();
    for (const auto& [e, pb_c, t_c, player_c] : view.each()) {
      const b2BodyType type = b2Body_GetType(pb_c.id);
      if (type == b2_staticBody)
        continue;

      // const auto meters_per_second = 0.1f;
      const auto force = move_force * b2Vec2{ l_input.x, l_input.y };
      b2Body_ApplyLinearImpulseToCenter(pb_c.id, force, true);

      break;
    }
  }
  */

  // Apply jump force
  {
    entt::entity first_dynamic_e = entt::null;
    auto view = r.view<const PhysicsBodyComponent, const TransformComponent, const PlayerComponent>();
    for (const auto& [e, pb_c, t_c, player_c] : view.each()) {
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
      const auto shape_eid_a = (entt::entity)(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(a)));
      const auto shape_eid_b = (entt::entity)(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(b)));

      assert(shape_eid_a != entt::null);
      assert(shape_eid_a != entt::null);
      callback(shape_eid_a, shape_eid_b);
    };

    const b2ContactEvents c_events = b2World_GetContactEvents(data->world_id);
    const b2SensorEvents s_events = b2World_GetSensorEvents(data->world_id);
    auto& ui_data = data->ui_data;
    ui_data.n_sensor_events = s_events.beginCount + s_events.endCount;
    ui_data.n_contact_events = c_events.beginCount + c_events.endCount;

    for (int i = 0; i < c_events.beginCount; ++i) {
      b2ContactBeginTouchEvent* beginEvent = c_events.beginEvents + i;
      const auto callback = [](const entt::entity e_a, const entt::entity e_b) {
        SINGLE_Events::get().dispatcher.trigger(OnCollisionEnter{ .shape_a = e_a, .shape_b = e_b });
      };
      convert_box2d_coll_to_entt(r, beginEvent->shapeIdA, beginEvent->shapeIdB, callback);
    }
    for (int i = 0; i < c_events.endCount; ++i) {
      b2ContactEndTouchEvent* endEvent = c_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->shapeIdA) && b2Shape_IsValid(endEvent->shapeIdB)) {
        const auto callback = [](const entt::entity e_a, const entt::entity e_b) {
          SINGLE_Events::get().dispatcher.trigger(OnCollisionExit{ .shape_a = e_a, .shape_b = e_b });
        };
        convert_box2d_coll_to_entt(r, endEvent->shapeIdA, endEvent->shapeIdB, callback);
      }
    }
    for (int i = 0; i < s_events.beginCount; ++i) {
      b2SensorBeginTouchEvent* beginEvent = s_events.beginEvents + i;
      const auto callback = [](const entt::entity e_a, const entt::entity e_b) {
        SINGLE_Events::get().dispatcher.trigger(OnCollisionEnter{ .shape_a = e_a, .shape_b = e_b });
      };
      convert_box2d_coll_to_entt(r, beginEvent->sensorShapeId, beginEvent->visitorShapeId, callback);
    }
    for (int i = 0; i < s_events.endCount; ++i) {
      b2SensorEndTouchEvent* endEvent = s_events.endEvents + i;
      if (b2Shape_IsValid(endEvent->sensorShapeId) && b2Shape_IsValid(endEvent->visitorShapeId)) {
        const auto callback = [](const auto e_a, const auto e_b) {
          SINGLE_Events::get().dispatcher.trigger(OnCollisionExit{ .shape_a = e_a, .shape_b = e_b });
        };
        convert_box2d_coll_to_entt(r, endEvent->sensorShapeId, endEvent->visitorShapeId, callback);
      }
    }
    SINGLE_Events::get().dispatcher.update();
  }
};

void
game_update(GameData* data)
{
  const auto evts = data->events;
  auto& r = internal_r;
  auto dt = data->dt;

  //
  // input events
  //

  static SINGLE_Inputs inputs_c;
  generate_button_state(evts, inputs_c);

  if (get_key_down(inputs_c, SDL_SCANCODE_SPACE))
    jump |= true;
  if (get_key_down(inputs_c, SDL_SCANCODE_RETURN))
    pickup |= true;
  if (get_key_down(inputs_c, SDL_SCANCODE_KP_9))
    create_empty<Request_GameOver>(r);

  /*
if (get_key_down(inputs_c, SDL_SCANCODE_W))
  keyboard_l.y = -1;
if (get_key_down(inputs_c, SDL_SCANCODE_S))
  keyboard_l.y = 1;
if (get_key_down(inputs_c, SDL_SCANCODE_A))
  keyboard_l.x = -1;
if (get_key_down(inputs_c, SDL_SCANCODE_D))
  keyboard_l.x = 1;
if (get_key_down(inputs_c, SDL_SCANCODE_UP))
  keyboard_r.y = -1;
if (get_key_down(inputs_c, SDL_SCANCODE_DOWN))
  keyboard_r.y = 1;
if (get_key_down(inputs_c, SDL_SCANCODE_LEFT))
  keyboard_r.x = -1;
if (get_key_down(inputs_c, SDL_SCANCODE_RIGHT))
  keyboard_r.x = 1;
if (get_key_up(inputs_c, SDL_SCANCODE_W))
  keyboard_l.y = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_S))
  keyboard_l.y = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_A))
  keyboard_l.x = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_D))
  keyboard_l.x = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_UP))
  keyboard_r.y = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_DOWN))
  keyboard_r.y = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_LEFT))
  keyboard_r.x = 0;
if (get_key_up(inputs_c, SDL_SCANCODE_RIGHT))
  keyboard_r.x = 0;
*/

  data->mouse_dt = vec2{ 0, 0 };

  for (const SDL_Event& evt : evts) {
    if (evt.type == SDL_EVENT_KEY_DOWN) {
      const auto scancode = evt.key.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = evt.key.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyDown %s %i %i", scancode_name, down, repeat);
    }
    if (evt.type == SDL_EVENT_KEY_UP) {
      const SDL_KeyboardEvent& k_evt = evt.key;
      const auto scancode = k_evt.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = k_evt.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyUp %s %i %i", scancode_name, down, repeat);
    }

    //
    // mouse events
    //
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      SDL_MouseButtonEvent m_evt = evt.button;

      if (m_evt.button == SDL_BUTTON_LEFT) {
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

      if (m_evt.button == SDL_BUTTON_RIGHT) {
        spawn(data, data->mouse_pos, { stuff_size, stuff_size }, { 1.0, 1.0, 1.0 });
      }
    }
    if (evt.type == SDL_EVENT_MOUSE_MOTION) {
      SDL_MouseMotionEvent m_evt = evt.motion;
      // data->mouse_pos = { m_evt.x, m_evt.y };
      data->mouse_dt = { m_evt.xrel, m_evt.yrel };
    }
    // SDL_Log("mouse_dt: (%f, %f)", data->mouse_dt.x, data->mouse_dt.y);

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
  // ui_data.keyboard_l = keyboard_l;
  // ui_data.keyboard_r = keyboard_r;

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

    const float epsilon = 0.01f;
    if (abs(controller_l.x) < epsilon)
      controller_l.x = 0.0f;
    if (abs(controller_l.y) < epsilon)
      controller_l.y = 0.0f;
    if (abs(controller_r.x) < epsilon)
      controller_r.x = 0.0f;
    if (abs(controller_r.y) < epsilon)
      controller_r.y = 0.0f;

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
    // jump |= s_press;
    pickup |= s_press;

    ui_data.controller_l.x = lx_nrm;
    ui_data.controller_l.y = ly_nrm;
    ui_data.controller_r.x = rx_nrm;
    ui_data.controller_r.y = ry_nrm;
    break;
  }

  // l_input = keyboard_l + controller_l;
  // r_input = keyboard_r + controller_r;

  // // clamp the inputs
  // l_input.x = std::clamp(l_input.x, -1.0f, 1.0f);
  // l_input.y = std::clamp(l_input.y, -1.0f, 1.0f);
  // r_input.x = std::clamp(r_input.x, -1.0f, 1.0f);
  // r_input.y = std::clamp(r_input.y, -1.0f, 1.0f);

  // set camera to position of transform
  // {
  //   auto view = r.view<const PhysicsBodyComponent, const TransformComponent, const PlayerComponent>();
  //   for (const auto& [e, pb_c, t_c, player_c] : view.each()) {
  //     const b2BodyType type = b2Body_GetType(pb_c.id);
  //     if (type == b2_staticBody)
  //       continue;
  //     camera_pos = meters_to_pixels(b2Body_GetPosition(pb_c.id)) - 0.5 * screen_size;
  //     break;
  //   }
  // }
  // SDL_Log("camera_pos: %0.2f, %0.2f", camera_pos.x, camera_pos.y);

  // update camera with right analogue
  // camera_pos = camera_pos + data->dt * camera_speed * r_input;
  // data->camera_pos = camera_pos;

  auto& camera_c = data->camera_c;
  auto& camera_pos = data->camera_pos;

  if (capture_mouse) {
    auto mouse_input = data->mouse_dt;
    const float mouse_input_x = mouse_input.x * mouse_sens;
    camera_c.yaw += mouse_input_x;
    const float mouse_input_y = mouse_input.y * mouse_sens;
    camera_c.pitch += mouse_input_y;
    if (camera_c.pitch > 89.0f)
      camera_c.pitch = 89.0f;
    if (camera_c.pitch < -89.0f)
      camera_c.pitch = -89.0f;
  }

  // // const auto fwd_dir = glm::rotate(vec3_to_quat(t.rotation), forward);
  // const auto fwd_dir = forward;

  // update camera pos with wasd
  const float velocity = camera_speed * dt;
  if (get_key_held(inputs_c, SDL_SCANCODE_W))
    camera_pos += get_forward_dir(camera_c) * velocity;
  if (get_key_held(inputs_c, SDL_SCANCODE_S))
    camera_pos -= get_forward_dir(camera_c) * velocity;
  if (get_key_held(inputs_c, SDL_SCANCODE_A))
    camera_pos -= get_right_dir(camera_c) * velocity;
  if (get_key_held(inputs_c, SDL_SCANCODE_D))
    camera_pos += get_right_dir(camera_c) * velocity;
  if (get_key_held(inputs_c, SDL_SCANCODE_SPACE))
    camera_pos += get_up_dir(camera_c) * velocity;
  if (get_key_held(inputs_c, SDL_SCANCODE_LSHIFT))
    camera_pos -= get_up_dir(camera_c) * velocity;
  if (get_key_down(inputs_c, SDL_SCANCODE_M)) {
    capture_mouse = !capture_mouse;
    // SDL_CaptureMouse(capture_mouse);
    // if (SDL_CursorVisible() && capture_mouse)
    //   SDL_HideCursor();
    // else
    //   SDL_ShowCursor();
    SDL_Log("capture mouse: %i", capture_mouse);
  }

  camera_c.view =
    calculate_perspective_view(TransformComponent{ .pos = { camera_pos.x, camera_pos.y, camera_pos.z } }, camera_c);

  // Update transforms via physics body.
  update_transforms_from_physics(r);

  // update_events_system()
  SINGLE_Events::get().dispatcher.update();

  // check what you're colliding with.
  // if it's a producer: you gain 1 item.
  // if it's a consumer, you lose 1 item.
  if (jump) {
    // TODO
    // b2Shape_TestPoint(b2ShapeId shapeId, b2Vec2 point)
  }

  //
  // systems
  //

  // process ui data.
  const auto view = r.view<const Request_GameOver>();
  const bool gameover = view.size() > 0;
  if (data->ui_data.play_again && gameover) {
    SDL_Log("(gamethread) ui clicked to play again");
    r.destroy(view.begin(), view.end());
    game_refresh(data);
    game_init(data);
  }

  // populate game's copy of ui data from the gamethread
  {
    auto& hmm = ui_data.hmm;
    hmm.clear(); // .clear() is bad
    // const auto view = r.view<const TransformComponent, const ColourComponent, const InventoryComponent>();
    // for (const auto& [e, t_c, col_c, inv_c] : view.each())
    //   hmm.push_back(UIEntity{ .entity = e, .renderable = { .transform = t_c, .colour = col_c }, .inventory = inv_c });

    ui_data.play_again = false;
    ui_data.game_over = gameover;
  }
};

void
game_update_ui(GameUIData* ui_data)
{
  ImGui::SetCurrentContext(ui_data->ctx);
  const auto& data = ui_data->ui_data;

  // int controllers = 0;
  // SDL_GetJoysticks(&controllers);
  // ImGui::Text("n_controllers: %i", controllers);

  {
    auto flags = 0;
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin("SomeOtherCrazyWindow", nullptr, flags);

    if (data.game_dt != 0.0f)
      ImGui::Text("(GameThread) FPS: %f", 1.0f / data.game_dt);
    else
      ImGui::Text("(GameThread) FPS: dt not set?");

    ImGui::Text("(RenderThread) FPS: %0.2f", ImGui::GetIO().Framerate);
    ImGui::Text("contact events: %i", data.n_contact_events);
    ImGui::Text("sensor events: %i", data.n_sensor_events);
    ImGui::Text("renderables: %i", (int)ui_data->renderable.size());
    ImGui::Text("ui data hmm: %i", (int)ui_data->ui_data.hmm.size());
    ImGui::Text("camera_pos: %0.2f, %0.2f, %0.2f", ui_data->camera_pos.x, ui_data->camera_pos.y, ui_data->camera_pos.z);
    ImGui::Text("camera (pitch) %0.2f, (yaw) %0.2f", ui_data->camera_c.pitch, ui_data->camera_c.yaw);

    ImGui::End();
  }

  {
    auto flags = 0;
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_AlwaysAutoResize;
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
    // ImGui::Text("%f %f %f %f", l_input.x, l_input.y, r_input.x, r_input.y);

    ImGui::Text("MouseInput");
    auto& io = ImGui::GetIO();
    ImGui::Text("%f %f", io.MousePos.x, io.MousePos.y);

    ImGui::End();
  }

  // systems
  update_ui_gameover_system(ui_data->ui_data);

  // Worldspace overlay.
  /*
  {
    ImGuiWindowFlags flags = 0;
    flags |= ImGuiWindowFlags_NoDecoration;
    flags |= ImGuiWindowFlags_NoMove;
    flags |= ImGuiWindowFlags_NoBackground;
    flags |= ImGuiWindowFlags_NoFocusOnAppearing;
    flags |= ImGuiWindowFlags_NoDocking;
    flags |= ImGuiWindowFlags_NoInputs;
    flags |= ImGuiWindowFlags_NoSavedSettings;
    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always, { 0.0f, 0.0f });
    ImGui::SetNextWindowSize({ screen_size.x, screen_size.y }, ImGuiCond_Always);
    ImGui::Begin("overlay", 0, flags);

    const auto camera_p = ui_data->ui_data.camera_pos;
    for (const auto& ui : ui_data->ui_data.hmm) {
      // ImGui::PushID(eid);

      // const auto pos = ui.renderable.transform.pos;
      // const auto ss_pos = worldspace_to_screenspace({ camera_p.x, camera_p.y }, { pos.x, pos.y }, screen_size);
      // ImGui::SetCursorScreenPos({ ss_pos.x, ss_pos.y });

      // const auto txt = std::format("eid: {} \n items: {}", (uint32_t)ui.entity, ui.inventory.items);
      // const auto txt = std::format("i: {}", ui.inventory.items);
      // ImGui::Text("%s", txt.c_str());

      // ImGui::PopID();
    }

    ImGui::End();
  }
  */
};

// note: game_init() is called after game_refresh();
void
game_refresh(GameData* data)
{
  SDL_Log("(GameEngine) game_refresh()");
  refreshed = true;

  // clear the registry
  internal_r.clear();

  // Delete the physics world. Create another one.
  b2DestroyWorld(data->world_id);
  data->world_id = {};

  // deletes all the physicsbody
  // auto& r = *data->r;
  // auto view = r.view<PhysicsBodyComponent>();
  // view.each([&r](const auto e, const auto& pb_c) { r.destroy(e); });
};

} // namespace game2d