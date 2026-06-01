#include "pch.hpp" // IWYU pragma: keep

#include "game.hpp"

#include "game_and_engine_interop.hpp"
#include "modules/actors/actor_player/actor_player_components.hpp"
#include "modules/box2d/box2d_components.hpp"
#include "modules/entt/entt_helpers.hpp"
#include "modules/input/input_helpers.hpp"
#include "modules/physics/box2d_parallel.hpp"
#include "modules/physics/physics_components.hpp"
#include "modules/physics/physics_system.hpp"
#include "modules/physics/render_helpers.hpp"
#include "modules/raws/raws_components.hpp"
#include "modules/renderer/renderer_helpers.hpp"
#include "systems/system_anims/anims_components.hpp"
#include "systems/system_anims/anims_helpers.hpp"
#include "systems/system_anims/anims_system.hpp"
#include "systems/system_input/input_system.hpp"
#include "systems/system_items/items_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_components.hpp"
#include "systems/ui_system_gameover/ui_gameover_system.hpp"

namespace game2d {

using namespace std::literals;

static entt::registry internal_r;
static bool refreshed = false;
static bool capture_mouse = false;
static float camera_speed = 500.0f;
static float mouse_sens = 0.01f;
static float stuff_size = 32.0f;
const auto jump_force = b2Vec2{ 0.0f, -5.0f };
const auto move_force = 0.15f;
const auto move_damping = 10.0f;

// Physics
static b2Vec2 gravity = { 0.0f, 0.0f };

const auto get_system_time_for_seed = []() -> uint64_t {
  auto now = std::chrono::high_resolution_clock::now();
  long long seed = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  return seed;
};

void
game_init(GameData* data)
{
  SDL_Log("game_init() - start");

  // sets as an instance of an entt::registry used by this dll
  data->r = &internal_r;
  auto& r = internal_r;

  auto& camera_pos = data->camera_pos;
  // camera_pos = { -7.5, 3.2, -4.45 };

  const auto view = r.view<const TransformComponent, const ColourComponent, const SpriteComponent>();
  SDL_Log("renderables: %zu", view.size_hint());

  // create physics world
  const auto worldId = emplace_or_replace_physics_world();
  SINGLE_Physics::get().worldId = worldId;
  SDL_Log("Created physics world.");

  // Load a texture.
  SDL_Log("Loading texture...");
  // const auto char_tex_path = "packs/PUNY_CHARACTERS_v2_1/Pre-Made Character Spritesheets/Basic Melee/Novice.png"s;
  const auto char_tex_path = "packs/PUNY_CHARACTERS_v2_1/premade.png"s;
  const auto char_tex = create_and_upload_gpu_texture(data->device, char_tex_path);
  data->unprocessed_textures.push_back(char_tex.texture);
  SDL_Log("(gamethread) Loaded %zu textures.", data->unprocessed_textures.size());

  // rnd_x on left side of screen.
  uint64_t seed = 0;
#if defined(_DEBUG)
  seed = get_system_time_for_seed();
#endif
  static RandomState rnd(seed);
  const auto rnd_0_x = random(rnd, 100.0f, 450.0f);

  for (int i = 0; i < 100; i++) {
    const auto rnd_x = random(rnd, 550.0f, 900.0f);
    const auto rnd_y = random(rnd, 0.0f, 720.0f);
    const auto consumer_e = spawn(r,
                                  {
                                    .pos = { rnd_x, rnd_y },
                                    .render_size = { stuff_size, stuff_size },
                                    .coll_size = { stuff_size, stuff_size },
                                    .is_occluder = true,
                                  });
    r.emplace<ContainerReceiverComponent>(consumer_e);
    r.emplace<InventoryComponent>(consumer_e, InventoryComponent{ .items = 0 });
  }

  // const auto provider_e = spawn(r,
  //                               {
  //                                 .pos = { rnd_0_x, 300 },
  //                                 .size = { stuff_size, stuff_size },
  //                                 .colour = { 1.0f, 0.0f, 0.0f },
  //                                 .is_emitter = true,
  //                               });
  // r.emplace<ContainerProviderComponent>(provider_e);
  // r.emplace<InventoryComponent>(provider_e, InventoryComponent{ .items = 5 });

  const auto player_body_def = PhysicsBodyDef{
    .linear_damping = move_damping,
  };
  const auto player_e = spawn(r,
                              SpawnConfig{ .pos = { 500.0f, 450.0f },
                                           .render_size = { 64.0f, 64.0f },
                                           .coll_size = { 32.0f, 32.0f },
                                           .body_def = player_body_def,
                                           .colour = { 1.0f, 1.0f, 1.0f },
                                           .sprite = default_character_spritesheet(),
                                           .is_emitter = true });
  r.emplace<PlayerComponent>(player_e);
  r.emplace<InventoryComponent>(player_e, InventoryComponent{ .items = 0 });
  r.emplace<SpriteAnimationState>(player_e);
  r.emplace<SpriteDirComponent>(player_e);

  for (int i = 0; i < 6; i++) {
    const auto rnd_x = random(rnd, 550.0f, 900.0f);
    const auto rnd_y = random(rnd, 0.0f, 720.0f);
    const auto light_e = spawn(r,
                               { .pos = { rnd_x, rnd_y },
                                 .render_size = { stuff_size, stuff_size },
                                 .coll_size = { stuff_size, stuff_size },
                                 .colour = { 1.0f, 0.0f, 0.0f, 1.0f },
                                 .sprite = default_spritesheet(),
                                 .is_emitter = true });
  }

  // spawn(r,
  //       { .pos = { 0.0f, 0.0f }, .size = { stuff_size, stuff_size }, .colour = { 1.0f, 1, 0, 1.0f }, .is_emitter = true
  //       });
  // spawn(r,
  //       { .pos = { 0.0f, 720.0f }, .size = { stuff_size, stuff_size }, .colour = { 1.0f, 1, 0, 1.0f }, .is_emitter = true
  //       });
  // spawn(
  //   r, { .pos = { 1280.0f, 0.0f }, .size = { stuff_size, stuff_size }, .colour = { 1.0f, 1, 0, 1.0f }, .is_emitter = true
  //   });
  // spawn(
  //   r,
  //   { .pos = { 1280.0f, 720.0f }, .size = { stuff_size, stuff_size }, .colour = { 1.0f, 1, 0, 1.0f }, .is_emitter = true
  //   });

  SDL_Log("game_init() - done");
};

void
game_fixed_update(GameData* data)
{
  auto& r = internal_r;

  const auto& inputs_c = SINGLE_FrameInput::get();

  const auto player_input = inputs_c.keyboard_l + inputs_c.controller_l;

  // Apply force to first dynamic body
  {
    auto view = r.view<const PhysicsBodyComponent, const TransformComponent, const PlayerComponent>();
    for (const auto& [e, pb_c, t_c, player_c] : view.each()) {
      const b2BodyType type = b2Body_GetType(pb_c.id);
      if (type == b2_staticBody)
        continue;

      // const auto meters_per_second = 0.1f;
      const auto force = move_force * b2Vec2{ player_input.x, player_input.y };
      b2Body_ApplyLinearImpulseToCenter(pb_c.id, force, true);

      break;
    }
  }

  // Apply jump force
  /*
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
  */

  // note: also see fixed_update() in main

  // static constexpr int MILLISECONDS_PER_FIXED_TICK = 7; // or ~142 ticks per second
  constexpr int MILLISECONDS_PER_FIXED_TICK = 16; // or ~62.5 ticks per second
  constexpr Uint64 NS_PER_FIXED_TICK = (Uint64)(MILLISECONDS_PER_FIXED_TICK * 1e6);
  constexpr float fixed_dt = MILLISECONDS_PER_FIXED_TICK / 1000.0f;
  update_physics_system(data, r, fixed_dt);

  // Update transforms via physics body.
  update_transforms_from_physics(r);
};

void
game_update(GameData* data)
{
  const auto evts = data->events;
  auto& r = internal_r;
  auto dt = data->dt;

  update_input_system(r, data);
  const auto& frame_inputs_c = SINGLE_FrameInput::get();

  auto& ui_data = data->ui_data;
  // ui_data.n_controllers = n_joysticks;
  ui_data.n_controllers = 0;
  ui_data.keyboard_l = frame_inputs_c.keyboard_l;
  ui_data.keyboard_r = frame_inputs_c.keyboard_r;
  ui_data.controller_l = frame_inputs_c.controller_l;
  ui_data.controller_r = frame_inputs_c.controller_r;

  // convert input <=> sprite
  const auto player_e = get_first<PlayerComponent>(r);
  if (player_e != entt::null) {
    auto& player_sprite_c = r.get<SpriteDirComponent>(player_e);

    auto dir = vec_to_dir(ui_data.keyboard_l.x, ui_data.keyboard_l.y, player_sprite_c.dir);
    player_sprite_c.dir = dir;

    // vel can impact animations
    auto body_id = r.get<PhysicsBodyComponent>(player_e).id;
    auto body_vel = b2Body_GetLinearVelocity(body_id);
    player_sprite_c.vel = { body_vel.x, body_vel.y };
  }

  // systems.
  update_animator_system(r, dt);

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
    auto& ui_data = data->ui_data;
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
    ImGui::Text("%0.2f %0.2f %0.2f %0.2f", data.keyboard_l.x, data.keyboard_l.y, data.keyboard_r.x, data.keyboard_r.y);

    ImGui::Text("Controllers");
    ImGui::Text("%i %f %f %f %f",
                data.n_controllers,
                data.controller_l.x,
                data.controller_l.y,
                data.controller_r.x,
                data.controller_r.y);

    const auto& input_c = SINGLE_Inputs::get();
    ImGui::Text("%zu down, %zu up, %zu held", input_c.keys_down.size(), input_c.keys_up.size(), input_c.keys_held.size());

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
  // internal_r.clear();
};

void
game_shutdown(GameData* data)
{
  SDL_Log("(Game) game_shutdown()");

  internal_r.clear();

  physics_shutdown();

  data->r = nullptr;
}

} // namespace game2d