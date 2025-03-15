#include "game.hpp"

#include "box2d/math_functions.h"
#include "common.hpp"
#include "render_helpers.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <box2d/box2d.h>
#include <imgui.h>

namespace game2d {

static bool refreshed = false;
static vec2 input{ 0, 0 };

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

  create_something(data, { 300, 300 });
  create_something(data, { 600, 600 });
};

void
game_fixed_update(GameData* data)
{
  auto& r = data->r;

  // apply force to physics objects
  // for (const auto& [e, physics_c] : r.view<PhysicsBodyComponent>().each()) {
  // }

  // apply force to first
  auto view = data->r.view<const PhysicsBodyComponent, const TransformComponent>();
  auto first_e = view.front();
  if (first_e != entt::null) {
    const auto& pb_c = data->r.get<PhysicsBodyComponent>(first_e);
    const auto meters_per_second = 0.1f;
    const auto force = b2Body_GetMass(pb_c.id) * meters_per_second * b2Vec2{ input.x, input.y };
    b2Body_ApplyLinearImpulseToCenter(pb_c.id, force, true);
  }
};

void
game_update(GameData* data)
{
  const auto evts = data->events;

  // note: remove static at some point
  // static float timer_cur = 0.0f;
  // static float timer_max = 0.1f;
  // timer_cur += data->dt;
  // if (timer_cur >= timer_max) {
  //   SDL_Log("X seconds elapsed...");
  //   timer_cur -= timer_max;
  // }

  for (const SDL_Event& evt : evts) {
    if (evt.type == SDL_EVENT_KEY_DOWN) {
      const auto scancode = evt.key.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = evt.key.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyDown %s %i %i", scancode_name, down, repeat);

      if (scancode == SDL_SCANCODE_W)
        input.y = -1;
      if (scancode == SDL_SCANCODE_S)
        input.y = 1;
      if (scancode == SDL_SCANCODE_A)
        input.x = -1;
      if (scancode == SDL_SCANCODE_D)
        input.x = 1;
      if (scancode == SDL_SCANCODE_KP_0)
        create_something(data, data->mouse_pos);
    }
    if (evt.type == SDL_EVENT_KEY_UP) {
      const SDL_KeyboardEvent& k_evt = evt.key;
      const auto scancode = k_evt.scancode;
      const auto scancode_name = SDL_GetScancodeName(scancode);
      const auto down = k_evt.down;
      const auto repeat = evt.key.repeat;
      SDL_Log("(GameThread)(GameUpdate) KeyUp %s %i %i", scancode_name, down, repeat);

      if (scancode == SDL_SCANCODE_W)
        input.y = 0;
      if (scancode == SDL_SCANCODE_S)
        input.y = 0;
      if (scancode == SDL_SCANCODE_A)
        input.x = 0;
      if (scancode == SDL_SCANCODE_D)
        input.x = 0;
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
  }

  // Update transforms via physics body.
  update_transforms_from_physics(data->r);
};

void
game_update_ui(GameUIData* ui_data)
{
  ImGui::SetCurrentContext(ui_data->ctx);

  auto flags = 0;
  flags |= ImGuiWindowFlags_NoDecoration;
  flags |= ImGuiWindowFlags_AlwaysAutoResize;

  ImGui::Begin("SomeCrazyWindow", nullptr, flags);
  ImGui::Text("(GameThread) FPS: %0.2f", 1.0f / ui_data->game_dt);
  ImGui::Text("(RenderThread) FPS: %0.2f", ImGui::GetIO().Framerate);
  ImGui::End();
};

void
game_refresh(GameData* data)
{
  SDL_Log("(GameEngine) game_refresh()");
  refreshed = true;

  // Delete the physics world. Create another one.
  b2DestroyWorld(data->world_id);
  b2WorldDef world_def = b2DefaultWorldDef();
  world_def.gravity = { 0.0f, 0.0f };
  world_def.enableSleep = true;
  data->world_id = b2CreateWorld(&world_def);

  // cleanup physics components
  auto view = data->r.view<PhysicsBodyComponent>();
  data->r.destroy(view.begin(), view.end());

  // GameData& data_ref = *your_ref_to_data;
  // entt::registry& r = your_ref_to_data->r;
  // auto view = r->view<const TransformComponent>();
  // const auto info = std::format("(GameEngine) Transforms: {}", view.size());
  // SDL_Log("%s", info.c_str());
};

} // namespace game2d