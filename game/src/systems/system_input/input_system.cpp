#include "input_system.hpp"

#include "input_components.hpp"
#include "modules/actors/actor_player/actor_player_components.hpp"
#include "modules/maths/helpers.hpp"
#include "modules/maths/vec.hpp"
#include "systems/system_input/input_helpers.hpp"

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

std::array<float, 4>
process_joystick_analogues(SDL_Joystick* instance)
{
  const auto lx_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_LEFTX);
  const auto ly_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_LEFTY);
  const auto rx_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_RIGHTX);
  const auto ry_raw = SDL_GetJoystickAxis(instance, SDL_GAMEPAD_AXIS_RIGHTY);

  auto lx_nrm = scale(lx_raw, -32768, 32767, -1.0f, 1.0f);
  auto ly_nrm = scale(ly_raw, -32768, 32767, -1.0f, 1.0f);
  auto rx_nrm = scale(rx_raw, -32768, 32767, -1.0f, 1.0f);
  auto ry_nrm = scale(ry_raw, -32768, 32767, -1.0f, 1.0f);
  const vec2 inp_l = vec2{ lx_nrm, ly_nrm };
  const vec2 inp_r = vec2{ rx_nrm, ry_nrm };

  const float deadzone = 0.05f;

  if (abs(lx_nrm) < deadzone)
    lx_nrm = 0.0f;
  if (abs(ly_nrm) < deadzone)
    ly_nrm = 0.0f;
  if (abs(rx_nrm) < deadzone)
    rx_nrm = 0.0f;
  if (abs(ry_nrm) < deadzone)
    ry_nrm = 0.0f;

  return { lx_nrm, ly_nrm, rx_nrm, ry_nrm };
}

void
update_input_system(entt::registry& r, const GameData* data)
{
#if defined(_DEBUG)
  // ZoneScoped;
#endif

  const auto evts = data->events;
  auto& inputs_c = SINGLE_Inputs::get();
  generate_button_state(evts, inputs_c);
  inputs_c.mouse_dt = { 0.0f, 0.0f };

  int n_joysticks = 0;
  auto* joysticks = SDL_GetJoysticks(&n_joysticks);
  inputs_c.n_joysticks = n_joysticks;

  for (const auto& [e, player_c, input_c] : r.view<PlayerComponent, InputComponent>().each()) {

    input_c.lx = 0.0f;
    input_c.ly = 0.0f;
    input_c.rx = 0.0f;
    input_c.ry = 0.0f;
    input_c.shoot_down = false;

    input_c.ry += get_key_held(inputs_c, SDL_SCANCODE_UP) ? -1.0f : 0.0f;
    input_c.ry += get_key_held(inputs_c, SDL_SCANCODE_DOWN) ? 1.0f : 0.0f;
    input_c.rx += get_key_held(inputs_c, SDL_SCANCODE_LEFT) ? -1.0f : 0.0f;
    input_c.rx += get_key_held(inputs_c, SDL_SCANCODE_RIGHT) ? 1.0f : 0.0f;

    input_c.ly += get_key_held(inputs_c, SDL_SCANCODE_W) ? -1.0f : 0.0f;
    input_c.ly += get_key_held(inputs_c, SDL_SCANCODE_S) ? 1.0f : 0.0f;
    input_c.lx += get_key_held(inputs_c, SDL_SCANCODE_D) ? 1.0f : 0.0f;
    input_c.lx += get_key_held(inputs_c, SDL_SCANCODE_A) ? -1.0f : 0.0f;

    for (const auto& evt : data->events) {
      if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        SDL_MouseButtonEvent m_evt = evt.button;
        input_c.shoot_down = m_evt.button == SDL_BUTTON_LEFT;
      }
    }

    for (int i = 0; i < n_joysticks; i++) {
      const SDL_JoystickID id = joysticks[i]; // TODO: assign joystick properly to player
      const auto instance = SDL_GetJoystickFromID(id);
      const auto [lx_nrm, ly_nrm, rx_nrm, ry_nrm] = process_joystick_analogues(instance);
      input_c.lx += lx_nrm;
      input_c.ly += ly_nrm;
      input_c.rx += rx_nrm;
      input_c.ry += ry_nrm;
    }

    const vec2 al = sanetize({ input_c.lx, input_c.ly });
    const vec2 ar = sanetize({ input_c.rx, input_c.ry });
    input_c.lx = al.x;
    input_c.ly = al.y;
    input_c.rx = ar.x;
    input_c.ry = ar.y;

    break; // only one for moment
  }

  // data->mouse_dt = vec2{ 0.0f, 0.0f };

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
      inputs_c.mouse_dt = { m_evt.xrel, m_evt.yrel };
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

  SDL_free(joysticks);
}

} // namespace game2d