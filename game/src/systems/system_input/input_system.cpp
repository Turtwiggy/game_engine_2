#include "pch.hpp"

#include "input_system.hpp"

#include "modules/input/input_helpers.hpp"

namespace game2d {

vec2
sanetize(vec2 v)
{
  const auto clamp = [](float v, const float min, const float max) -> float { return std::min(std::max(v, min), max); };

  const auto normalize = [](const vec2 v) -> vec2 {
    const float len = glm::sqrt(v.x * v.x + v.y * v.y);
    if (len < 1.0f)
      return v; // no change needed, already within unit circle
    if (len > 0)
      return vec2{ v.x / len, v.y / len };
    return v;
  };

  v.x = clamp(v.x, -1.0f, 1.0f);
  v.y = clamp(v.y, -1.0f, 1.0f);

  v = normalize(v);

  return v;
};

void
update_input_system(entt::registry& r, GameData* data)
{
#if defined(_DEBUG)
  // ZoneScoped;
#endif

  const auto evts = data->events;
  auto& inputs_c = SINGLE_Inputs::get();
  generate_button_state(evts, inputs_c);

  auto& inputs = SINGLE_FrameInput::get();
  inputs.reset();

  inputs.jump = get_key_down(inputs_c, SDL_SCANCODE_SPACE);
  inputs.pickup = get_key_down(inputs_c, SDL_SCANCODE_RETURN);
  inputs.request_gameover = get_key_down(inputs_c, SDL_SCANCODE_KP_9);

  inputs.keyboard_r = { 0, 0 };
  inputs.keyboard_r.y += get_key_held(inputs_c, SDL_SCANCODE_UP) ? -1.0f : 0.0f;
  inputs.keyboard_r.y += get_key_held(inputs_c, SDL_SCANCODE_DOWN) ? 1.0f : 0.0f;
  inputs.keyboard_r.x += get_key_held(inputs_c, SDL_SCANCODE_LEFT) ? -1.0f : 0.0f;
  inputs.keyboard_r.x += get_key_held(inputs_c, SDL_SCANCODE_RIGHT) ? 1.0f : 0.0f;

  inputs.keyboard_l = { 0, 0 };
  inputs.keyboard_l.y += get_key_held(inputs_c, SDL_SCANCODE_W) ? -1.0f : 0.0f;
  inputs.keyboard_l.y += get_key_held(inputs_c, SDL_SCANCODE_S) ? 1.0f : 0.0f;
  inputs.keyboard_l.x += get_key_held(inputs_c, SDL_SCANCODE_A) ? -1.0f : 0.0f;
  inputs.keyboard_l.x += get_key_held(inputs_c, SDL_SCANCODE_D) ? 1.0f : 0.0f;

  inputs.keyboard_l = sanetize(inputs.keyboard_l);
  inputs.keyboard_r = sanetize(inputs.keyboard_r);

  data->mouse_dt = vec2{ 0, 0 };

  for (const SDL_Event& evt : evts) {

    // mouse events
    //

    if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      SDL_MouseButtonEvent m_evt = evt.button;

      // if (m_evt.button == SDL_BUTTON_LEFT) {
      //   // test if you click a shape
      //   const auto mouse_pos = data->mouse_pos;
      //   auto view = r.view<const PhysicsBodyComponent>();
      //   for (const auto& [e, pb_c] : view.each()) {
      //     auto shape_ids = get_shapes(pb_c.id);
      //     if (b2Shape_TestPoint(shape_ids[0], pixels_to_meters(mouse_pos))) {
      //       // Destroy now!
      //       b2DestroyBody(pb_c.id);
      //       r.destroy(e);
      //     }
      //   }
      // }

      // if (m_evt.button == SDL_BUTTON_RIGHT)
      //   spawn(r, data->mouse_pos, { stuff_size, stuff_size }, { 1.0, 1.0, 1.0 }, false, false);
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
  ui_data.keyboard_l = inputs.keyboard_l;
  ui_data.keyboard_r = inputs.keyboard_r;

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

  // clamp the inputs
  // l_input.x = std::clamp(l_input.x, -1.0f, 1.0f);
  // l_input.y = std::clamp(l_input.y, -1.0f, 1.0f);
  // r_input.x = std::clamp(r_input.x, -1.0f, 1.0f);
  // r_input.y = std::clamp(r_input.y, -1.0f, 1.0f);

  //
}

} // namespace game2d