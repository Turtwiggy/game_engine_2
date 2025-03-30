#include "game.hpp"

#include "common.hpp"
#include "render_helpers.hpp"

#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "entt/entity/fwd.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_log.h>
#include <box2d/box2d.h>
#include <imgui.h>

namespace game2d {

static bool refreshed = false;
static vec2 l_input{ 0, 0 };
static vec2 r_input{ 0, 0 };
static bool jump = false;

void
create_something(GameData* data, vec2 pos)
{
  b2Vec2 size_meters = pixels_to_meters({ 50, 50 });
  b2Polygon box = b2MakeBox(0.5f * size_meters.x, 0.5f * size_meters.y);

  b2BodyDef bodyDef = b2DefaultBodyDef();
  bodyDef.type = b2_dynamicBody;
  bodyDef.position = b2Vec2{ pixels_to_meters(pos) };
  bodyDef.fixedRotation = true;
  b2BodyId body_id_0 = b2CreateBody(data->world_id, &bodyDef);

  b2ShapeDef shapeDef = b2DefaultShapeDef();
  b2CreatePolygonShape(body_id_0, &shapeDef, &box);

  TransformComponent t_c;
  t_c.pos = meters_to_pixels({ bodyDef.position.x, bodyDef.position.y });
  t_c.size = meters_to_pixels(size_meters);
  auto e = data->r.create();
  data->r.emplace<TransformComponent>(e, t_c);
  data->r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ body_id_0 });
};

void
game_init(GameData* data)
{
  SDL_Log("(GameEngine) Init()");

  b2WorldDef world_def = b2DefaultWorldDef();
  // world_def.workerCount = worker_count;
  // world_def.enqueueTask = EnqueueTask;
  // world_def.finishTask = FinishTask;
  // world_def.userTaskContext = s.get();
  world_def.gravity = { 0.0f, 10.0f };
  world_def.enableSleep = true;
  data->world_id = b2CreateWorld(&world_def);

  create_something(data, { 300, 300 });
  create_something(data, { 600, 300 });

  {
    b2Vec2 size_meters = pixels_to_meters({ 1000, 50 });
    b2Polygon box = b2MakeBox(0.5f * size_meters.x, 0.5f * size_meters.y);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = b2Vec2{ pixels_to_meters({ 1280 * 0.5f, 720 * 0.75f }) };
    bodyDef.fixedRotation = true;
    b2BodyId body_id_0 = b2CreateBody(data->world_id, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(body_id_0, &shapeDef, &box);

    TransformComponent t_c;
    t_c.pos = meters_to_pixels({ bodyDef.position.x, bodyDef.position.y });
    t_c.size = meters_to_pixels(size_meters);
    auto e = data->r.create();
    data->r.emplace<TransformComponent>(e, t_c);
    data->r.emplace<PhysicsBodyComponent>(e, PhysicsBodyComponent{ body_id_0 });
  }
};

void
game_fixed_update(GameData* data)
{
  entt::registry& r = data->r;

  // apply force to physics objects
  // for (const auto& [e, physics_c] : r.view<PhysicsBodyComponent>().each()) {
  // }

  auto view = data->r.view<const PhysicsBodyComponent, const TransformComponent>();
  for (const auto& [e, pb_c, t_c] : view.each()) {
    const b2BodyType type = b2Body_GetType(pb_c.id);
    if (type == b2_staticBody)
      continue;
    const auto meters_per_second = 0.1f;
    const auto force = b2Body_GetMass(pb_c.id) * meters_per_second * b2Vec2{ l_input.x, l_input.y };
    b2Body_ApplyLinearImpulseToCenter(pb_c.id, force, true);
    break; // apply force to first dynamic body
  }
};

void
game_update(GameData* data)
{
  const auto evts = data->events;
  entt::registry& r = data->r;

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
        l_input.y = -1;
      if (scancode == SDL_SCANCODE_S)
        l_input.y = 1;
      if (scancode == SDL_SCANCODE_A)
        l_input.x = -1;
      if (scancode == SDL_SCANCODE_D)
        l_input.x = 1;
      if (scancode == SDL_SCANCODE_KP_0)
        create_something(data, data->mouse_pos);
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
        l_input.y = 0;
      if (scancode == SDL_SCANCODE_S)
        l_input.y = 0;
      if (scancode == SDL_SCANCODE_A)
        l_input.x = 0;
      if (scancode == SDL_SCANCODE_D)
        l_input.x = 0;
    }
    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      SDL_MouseButtonEvent m_evt = evt.button;

      // test if you click a shape
      const auto mouse_pos = data->mouse_pos;
      auto view = data->r.view<const PhysicsBodyComponent>();
      for (const auto& [e, pb_c] : view.each()) {
        auto shape_ids = get_shapes(pb_c.id);
        if (b2Shape_TestPoint(shape_ids[0], pixels_to_meters(mouse_pos))) {

          // Destroy now!
          b2DestroyBody(pb_c.id);
          data->r.destroy(e);
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

  int n_joysticks = 0;
  auto* joysticks = SDL_GetJoysticks(&n_joysticks);

  auto& ui_data = data->ui_data;
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

    l_input = { lx_nrm, ly_nrm };
    r_input = { rx_nrm, ry_nrm };

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

  entt::entity first_dynamic_e = entt::null;
  {
    auto view = data->r.view<const PhysicsBodyComponent, const TransformComponent>();
    for (const auto& [e, pb_c, t_c] : view.each()) {
      const b2BodyType type = b2Body_GetType(pb_c.id);
      if (type == b2_staticBody)
        continue;
      first_dynamic_e = e;
      break;
    }
  }

  if (first_dynamic_e != entt::null) {
    // Apply jump force
    // Note: this should be being applied in fixed update not update
    if (jump) {
      auto& pb_c = r.get<PhysicsBodyComponent>(first_dynamic_e);
      b2Body_ApplyLinearImpulseToCenter(pb_c.id, { 0, -10 }, true);
    }
    jump = false;
  }

  // Update transforms via physics body.
  update_transforms_from_physics(r);
};

void
game_update_ui(GameUIData* ui_data)
{
  ImGui::SetCurrentContext(ui_data->ctx);
  const auto& data = ui_data->ui_data;

  auto flags = 0;
  flags |= ImGuiWindowFlags_NoDecoration;
  flags |= ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::Begin("SomeCrazyWindow", nullptr, flags);
  ImGui::Text("(GameThread) FPS: %0.2f", 1.0f / data.game_dt);
  ImGui::Text("(RenderThread) FPS: %0.2f", ImGui::GetIO().Framerate);
  ImGui::End();

  // int controllers = 0;
  // SDL_GetJoysticks(&controllers);
  // ImGui::Text("n_controllers: %i", controllers);

  ImGui::Begin("SomeOtherWindow", nullptr, flags);
  ImGui::Text("Controller");

  ImGui::Text("%i %f %f %f %f",
              data.n_controllers,
              data.controller_l.x,
              data.controller_l.y,
              data.controller_r.x,
              data.controller_r.y);

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

  // cleanup physics components
  auto& r = data->r;
  const auto view = r.view<PhysicsBodyComponent>();
  SDL_Log("Destroying %zu PhysicsBodyComponent", view.size());

  for (const auto& [e, pb_c] : view.each()) {
    r.destroy(e);
  }
  // r.destroy(view.begin(), view.end());

  // GameData& data_ref = *your_ref_to_data;
  // entt::registry& r = your_ref_to_data->r;
  // auto view = r->view<const TransformComponent>();
  // const auto info = std::format("(GameEngine) Transforms: {}", view.size());
  // SDL_Log("%s", info.c_str());
};

} // namespace game2d