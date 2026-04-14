#include "pch.hpp"

#include "game.hpp"

#include "game_and_engine_interop.hpp"
#include "modules/actors/actor_player/actor_player_components.hpp"
#include "modules/box2d/box2d_components.hpp"
#include "modules/box2d/box2d_helpers.hpp"
#include "modules/camera/perspective_helpers.hpp"
#include "modules/entt/entt_helpers.hpp"
#include "modules/events/events_core/events_components.hpp"
#include "modules/input/input_helpers.hpp"
#include "modules/physics/box2d_parallel.hpp"
#include "modules/physics/physics_components.hpp"
#include "modules/physics/physics_system.hpp"
#include "modules/physics/render_helpers.hpp"
#include "modules/raws/raws_components.hpp"
#include "systems/system_items/items_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_system.hpp"

namespace game2d {

static entt::registry internal_r;
static bool refreshed = false;
const auto screen_size = vec2(1280, 720); // todo: fix this

static bool capture_mouse = false;
static float camera_speed = 500.0f;
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

// Physics
static b2Vec2 gravity = { 0.0f, 0.0f };

const auto get_system_time_for_seed = []() -> int {
  auto now = std::chrono::high_resolution_clock::now();
  long long seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  return seed;
};

void
game_init(GameData* data)
{
  SDL_Log("(Game) Init()");

  // sets as an instance of an entt::registry used by this dll
  data->r = &internal_r;
  auto& r = internal_r;

  auto& camera_pos = data->camera_pos;
  camera_pos = { -7.5, 3.2, -4.45 };

  {
    const auto& view = r.view<const TransformComponent, const ColourComponent, const SpriteComponent>();
    SDL_Log("renderables: %zu", view.size_hint());
  }

  // create physics world
  SINGLE_Physics physics_c;
  physics_c.worldId = emplace_or_replace_physics_world();
  r.ctx().emplace<SINGLE_Physics>(physics_c);
  // create_empty<SINGLE_Physics>(r, physics_c);
  // r.emplace<Persistent>(get_first<SINGLE_Physics>(r));

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
    const auto consumer_e = spawn(r, { rnd_x, rnd_y }, { stuff_size, stuff_size }, { 0.0f, 1.0f, 0.0f });
    r.emplace<ContainerReceiverComponent>(consumer_e);
    r.emplace<InventoryComponent>(consumer_e, InventoryComponent{ .items = 0 });
  }

  const auto provider_e = spawn(r, { rnd_0_x, 300 }, { stuff_size, stuff_size }, { 1.0f, 0.0f, 0.0f });
  r.emplace<ContainerProviderComponent>(provider_e);
  r.emplace<InventoryComponent>(provider_e, InventoryComponent{ .items = 5 });

  const auto player_e = spawn(r, { 500, 450 }, { stuff_size, stuff_size }, { 0.0f, 1.0f, 1.0f }, false, true);
  r.emplace<PlayerComponent>(player_e);
  r.emplace<InventoryComponent>(player_e, InventoryComponent{ .items = 0 });
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

  const uint64_t ms_dt = data->dt_ns / 1000;
  update_physics_system(data, r, ms_dt);

  // Update transforms via physics body.
  update_transforms_from_physics(r);
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

      if (m_evt.button == SDL_BUTTON_RIGHT)
        spawn(r, data->mouse_pos, { stuff_size, stuff_size }, { 1.0, 1.0, 1.0 });
    }
    if (evt.type == SDL_EVENT_MOUSE_MOTION) {
      SDL_MouseMotionEvent m_evt = evt.motion;
      // data->mouse_pos = { m_evt.x, m_evt.y };
      data->mouse_dt = { m_evt.xrel, m_evt.yrel };
    }

    // joysticks
    //
    if (evt.type == SDL_EVENT_JOYSTICK_ADDED) {
      auto joystick_id = evt.jdevice.which;
      SDL_Log("Joystick Added: %i", joystick_id);
    }
    if (evt.type == SDL_EVENT_JOYSTICK_REMOVED) {
      auto joystick_id = evt.jdevice.which;
      SDL_Log("Joystick Removed: %i", joystick_id);
    }
    if (evt.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN) {
      SDL_Log("Joystick button down");
    }
    if (evt.type == SDL_EVENT_JOYSTICK_BUTTON_UP) {
      SDL_Log("Joystick button up");
    }
  }

  auto& ui_data = data->ui_data;

  /*

  // ui_data.keyboard_l = keyboard_l;
  // ui_data.keyboard_r = keyboard_r;

  /*
  int n_joysticks = 0;
  auto* joysticks = SDL_GetJoysticks(&n_joysticks);
  {
    // SDL_free(joysticks);
  }
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

  SDL_free(joysticks);
  */

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

  /*
  auto& camera_c = data->camera_c;
  auto& camera_pos = data->camera_pos;

  // if (capture_mouse) {
  //   auto mouse_input = data->mouse_dt;
  //   const float mouse_input_x = mouse_input.x * mouse_sens;
  //   camera_c.yaw += mouse_input_x;
  //   const float mouse_input_y = mouse_input.y * mouse_sens;
  //   camera_c.pitch += mouse_input_y;
  //   if (camera_c.pitch > 89.0f)
  //     camera_c.pitch = 89.0f;
  //   if (camera_c.pitch < -89.0f)
  //     camera_c.pitch = -89.0f;
  // }

  // // const auto fwd_dir = glm::rotate(vec3_to_quat(t.rotation), forward);
  // const auto fwd_dir = forward;

  // update camera pos with wasd
  const float velocity = camera_speed * dt;

  // 3d camera
  // if (get_key_held(inputs_c, SDL_SCANCODE_W))
  //   camera_pos += get_forward_dir(camera_c) * velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_S))
  //   camera_pos -= get_forward_dir(camera_c) * velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_A))
  //   camera_pos -= get_right_dir(camera_c) * velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_D))
  //   camera_pos += get_right_dir(camera_c) * velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_SPACE))
  //   camera_pos += get_up_dir(camera_c) * velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_LSHIFT))
  //   camera_pos -= get_up_dir(camera_c) * velocity;

  // 2d camera
  // if (get_key_held(inputs_c, SDL_SCANCODE_UP))
  //   camera_pos.y -= velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_DOWN))
  //   camera_pos.y += velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_LEFT))
  //   camera_pos.x -= velocity;
  // if (get_key_held(inputs_c, SDL_SCANCODE_RIGHT))
  //   camera_pos.x += velocity;

  // if (get_key_down(inputs_c, SDL_SCANCODE_M)) {
  //   capture_mouse = !capture_mouse;
  //   // SDL_CaptureMouse(capture_mouse);
  //   // if (SDL_CursorVisible() && capture_mouse)
  //   //   SDL_HideCursor();
  //   // else
  //   //   SDL_ShowCursor();
  //   SDL_Log("capture mouse: %i", capture_mouse);
  // }

  camera_c.view =
    calculate_perspective_view(TransformComponent{ .pos = { camera_pos.x, camera_pos.y, camera_pos.z } }, camera_c);


  // update_events_system()
  // SINGLE_Events::get().dispatcher.update();

  // check what you're colliding with.
  // if it's a producer: you gain 1 item.
  // if it's a consumer, you lose 1 item.
  // if (jump) {
  // TODO
  // b2Shape_TestPoint(b2ShapeId shapeId, b2Vec2 point)
  // }

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
  */

  // populate game's copy of ui data from the gamethread
  {
    auto& hmm = ui_data.hmm;
    hmm.clear(); // .clear() is bad
    //   // const auto view = r.view<const TransformComponent, const ColourComponent, const InventoryComponent>();
    //   // for (const auto& [e, t_c, col_c, inv_c] : view.each())
    //   //   hmm.push_back(UIEntity{ .entity = e, .renderable = { .transform = t_c, .colour = col_c }, .inventory = inv_c
    //   }); ui_data.play_again = false; ui_data.game_over = gameover;
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
    // ImGui::Text("camera (pitch) %0.2f, (yaw) %0.2f", ui_data->camera_c.pitch, ui_data->camera_c.yaw);

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
  SDL_Log("(Game) game_refresh()");
  refreshed = true;

  // clear the registry
  internal_r.clear();
};

} // namespace game2d