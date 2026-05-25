#include "pch.hpp" // IWYU pragma: keep

#include "game_and_engine_interop.hpp"
#include "imgui_helpers.hpp"
#include "modules/renderer/renderer_helpers.hpp"
#include "modules/sdl/sdl_exception.hpp"
#include "modules/sdl/sdl_hot_reload_dll.hpp"
#include "modules/sdl/sdl_setup.hpp"
#include "modules/sdl/sdl_shader.hpp"
#include "render_passes.hpp"
#include "threadsafe_queue.hpp"

using namespace game2d;
using namespace std::literals;

// docs: https://vkguide.dev/docs/introduction/vulkan_execution/
// docs: https://vulkan-tutorial.com/Overview
// https://vulkan.lunarg.com/sdk/home

// examples: http://github.com/SaschaWillems/Vulkan?tab=readme-ov-file#compute-shader
// vulkan triangle: https://github.com/SaschaWillems/Vulkan/blob/master/examples/trianglevulkan13/trianglevulkan13.cpp
// look at this: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pipelines/pipelines.cpp
// cpu particles: https://github.com/SaschaWillems/Vulkan/blob/master/examples/particlesystem/particlesystem.cpp
// multithreading: https://github.com/SaschaWillems/Vulkan/blob/master/examples/multithreading/multithreading.cpp
// instancing: https://github.com/SaschaWillems/Vulkan/blob/master/examples/instancing/instancing.cpp
// pipeline stats: https://github.com/SaschaWillems/Vulkan/tree/master/examples/pipelinestatistics
// deferred shading: https://github.com/SaschaWillems/Vulkan/tree/master/examples/deferred
// deferred shading: https://learnopengl.com/Advanced-Lighting/Deferred-Shading
// gpu particles: https://github.com/SaschaWillems/Vulkan/tree/master/examples/computeparticles
// cloth sim: https://github.com/SaschaWillems/Vulkan/tree/master/examples/computecloth
// ui text rendering: https://github.com/SaschaWillems/Vulkan/tree/master/examples/textoverlay
// ui sdf rendering: https://github.com/SaschaWillems/Vulkan/tree/master/examples/distancefieldfonts
// effects: radial blur: https://github.com/SaschaWillems/Vulkan/blob/master/examples/radialblur/radialblur.cpp
// effects: bloom: https://github.com/SaschaWillems/Vulkan/tree/master/examples/bloom
// https://github.com/TheSpydog/SDL_gpu_examples

// Goals:
// SDL3; input (keyboard) & audio
// SDL3: filesystem.
// GameThread
// RenderThread
// profiling cpu: TracyClient.cpp is all you need.
// profiling gpu: RenderDoc

// static constexpr Uint64 NS_PER_FIXED_TICK = 7 * 1e6;  // or ~142 ticks per second
// static constexpr Uint64 NS_PER_FIXED_TICK = 16 * 1e6; // or ~62.5 ticks per second
static bool limit_fps = false;
static int fps_limit = 240;
static Uint32 window_w = 1600, window_h = 900;

// read/write buffers from game=>render thread
// gamethread will GetWriteBuffer()
// renderthread will GetReadBuffer()
// clang-format off
vec2 mouse_pos;
RenderData rend_data[2];
std::atomic<int> current_read_buffer = 0;
RenderData& GetWriteBuffer(){return rend_data[1 - current_read_buffer.load(std::memory_order_acquire)];};
RenderData& GetReadBuffer(){return rend_data[current_read_buffer.load(std::memory_order_acquire)];};
void SwapBuffers() {current_read_buffer.store(1 - current_read_buffer.load(), std::memory_order_release);};
// clang-format on

// data owned by game thread
GameData game_data;

// data owned by ui thread
static std::mutex game_ui_mtx;
static GameUIData game_ui_data;
static sdl_game_code game_code;

static EventQueue<SDL_Event> game_event_queue;
static EventQueue<SDL_Event> rend_event_queue;
static std::atomic_bool running(true);
static SDL_Window* window;
static SDL_GPUDevice* device;

static std::atomic_bool rebuild_shaders(false);

const auto calc_dt_ns = [](Uint64 now, Uint64& past) -> Uint64 {
  Uint64 dt_ns = now - past;
  dt_ns = std::min(dt_ns, Uint64(250 * 1e6)); // avoid spiral
  past = now;
  return dt_ns;
};

void
delete_hotreload_locked_dll()
{
  const auto locked_dll_path = std::format("{}GameDLL-hot-locked.dll", SDL_GetBasePath());
  SDL_Log("Deleting: %s", locked_dll_path.c_str());

  const auto removed = SDL_RemovePath(locked_dll_path.c_str());

  if (removed)
    SDL_Log("Deleted -locked dll...");
  else
    SDL_Log("Failed to delete: %s", SDL_GetError());
}

void
do_rebuild_dll(sdl_game_code& game_code, std::string src_dll, std::string dst_dll)
{
  ZoneScoped;

  // Rebuild the dll
  SDL_Log("Rebuild dll...");

  game_code.valid = false;

  const auto now = SDL_GetTicks();

  // rebuild_dll
  const auto build_script = "rebuild_dll.bat";
  const auto full_path = std::format("{}assets/scripts/{}", SDL_GetBasePath(), build_script);
  const auto cmd = std::format("{}", full_path);
  const int result = std::system(cmd.c_str());
  if (result != 0)
    SDL_Log("Build failed...");

  const auto after = SDL_GetTicks();
  const auto rebuild_shaders_took = after - now;
  SDL_Log("Build took %zu ms", (after - now));

  if (result == 0) {
    SDL_Log("Build success...");
    game_code.game_shutdown(&game_data);

    sdl_unload_game_code(game_code);
    delete_hotreload_locked_dll();
    sdl_load_game_code(game_code, src_dll, dst_dll);
    SDL_Log("Load DLL... (rebuilt=>true)");
  }

  game_code.rebuilt = true;
}

void
GameThread()
{
  const auto info_str = std::format("(GameThread) SDL_IsMainThread(): {}", SDL_IsMainThread());
  SDL_Log("%s", info_str.c_str());

  if (!game_code.valid) {
    throw std::runtime_error("Failed to load .dll");
    exit(SDL_APP_FAILURE);
  }

  game_code.game_init(&game_data);

  SDL_Log("(GameThread) -- done init");
  tracy::SetThreadName("GameThread");

  while (running) {
    ZoneScopedN("GameThread");

    static Uint64 game_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, game_past);
    const float dt = (float)(1e-9 * (float)dt_ns);
    game_data.dt = dt;
    game_data.dt_ns = dt_ns;

    // update the game_data's ui data from the game_ui_data struct
    {
      std::unique_lock<std::mutex> lock(game_ui_mtx);
      game_data.ui_data = game_ui_data.ui_data;
    }

    // pop all the events at once from a thread-safe buffer.
    {
      game_data.events = game_event_queue.dequeue_all();
      game_data.mouse_pos = mouse_pos;
    }

    // Check for rebuild
    if (game_code.rebuilt && game_code.valid) {
      SDL_Log("(GameThread) game_refresh()");
      game_code.game_refresh(&game_data);
      game_code.game_init(&game_data);
      game_code.rebuilt = false; // eat the rebuilt flag
    }

    // run physics at fixed timesteps
    static Uint64 accu = 0;
    constexpr int physics_substep_count = 4;
    constexpr float physics_dt = 1.0f / 60.0f;
    constexpr Uint64 NS_PER_FIXED_TICK = (Uint64)(1e9 / 60.0);
    accu += dt_ns;
    while (accu >= NS_PER_FIXED_TICK) {
      accu -= NS_PER_FIXED_TICK;

      // FixedUpdate()
      {
        ZoneScopedN("(GameThread) game_fixed_update()");
        if (game_code.valid)
          game_code.game_fixed_update(&game_data);
      }
    }

    // GameUpdate()
    {
      ZoneScopedN("(GameThread) game_update()");
      if (game_code.valid)
        game_code.game_update(&game_data);
    }

    // Ding ding! frame done. Update RenderData
    RenderData& wb = GetWriteBuffer();
    {
      ZoneScopedN("(GameThread) game_update_write()");
      std::scoped_lock<std::mutex> lock0(wb.mtx);

      // should do something better than .clear()
      wb.renderable.clear();
      wb.lights.clear();
      wb.ui_data.hmm.clear();

      if (game_code.valid && game_data.r != nullptr) {

        // copy transforms in to RenderData.
        const auto view =
          game_data.r->view<const TransformComponent, const ColourComponent, const SpriteComponent, const LightComponent>();

        view.each([&](entt::entity e, const auto& t_c, const auto& col_c, const auto& sprite_c, const auto& light_c) {
          wb.renderable.push_back(Renderable{
            .transform = t_c,
            .colour = col_c,
            .sprite = sprite_c,
            .light = light_c,
          });
        });

        const auto light_view = game_data.r->view<const TransformComponent, const LightComponent>();

        light_view.each([&](entt::entity e, const auto& t_c, const auto& light_c) {
          if (light_c.is_emitter) {

            // Center the lights
            wb.lights.push_back(Light{
              .pos_x = t_c.pos.x + t_c.size.x * 0.5f,
              .pos_y = t_c.pos.y + t_c.size.y * 0.5f,
              .pos_z = t_c.pos.z + t_c.size.z * 0.5f,
            });
          }
        });

        // copy anything else in to renderdata buffer.
        wb.camera_pos = game_data.camera_pos;
        wb.ui_data = game_data.ui_data;
        wb.ui_data.game_dt = dt;
      }
    }

    SwapBuffers();
    FrameMark; // frame done
  }

  // b2DestroyWorld(game_data.world_id);
};

typedef struct SpriteInstance
{
  float x, y, z;
  float rotation;
  float w, h, is_emitter, is_occluder;
  float tex_u, tex_v, tex_w, tex_h;
  float colour[4];
  float sprite_max_x, sprite_max_y;
  float sprite_pos_x, sprite_pos_y;
  float sprite_wh_x, sprite_wh_y;
  float p3, p4;
} SpriteInstance;

int
RecompileShaders()
{
  // Change working directory to assets/shaders/source and run compile.bat
  const auto shader_dir = std::format("{}assets/shaders/source", SDL_GetBasePath());
  const auto cmd = "cd \"" + shader_dir + "\" && compile.bat";
  const int result = std::system(cmd.c_str());
  if (result != 0)
    SDL_Log("(shaders) Rebuild failed...");
  if (result == 0)
    SDL_Log("(shaders) Rebuild success...");
  return result;
}

//
// note: if you get an error in RenderDoc about descriptor set not bound,
// make sure that e.g. the samplerCount in ShaderInput matches the expected Samplers in the shader.
//

const ShaderInput pull_vert_input{
  .shaderFilename = "PullSpriteBatch.vert",
  .samplerCount = 0,
  .uniformBufferCount = 1,
  .storageBufferCount = 1,
  .storageTextureCount = 0,
};

SDL_GPUGraphicsPipeline*
CreateErrorPipeline()
{
  const ShaderInput frag_input{
    .shaderFilename = "SolidColorInput.frag",
    .samplerCount = 0,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, SDL_GPU_SAMPLECOUNT_1);
};
SDL_GPUGraphicsPipeline*
CreateSpritePipeline(const SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1)
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteSheet.frag",
    .samplerCount = 1,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, sample_count);
};
SDL_GPUGraphicsPipeline*
CreateEmitterAndOccluderPipeline(const SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1)
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteLightingBase.frag",
    .samplerCount = 0,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, sample_count);
}
SDL_GPUGraphicsPipeline*
CreateLightingSeedPipeline()
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteLightingSeed.frag",
    .samplerCount = 1,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, SDL_GPU_SAMPLECOUNT_1);
}
SDL_GPUGraphicsPipeline*
CreateJumpfloodPipeline()
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteLightingJumpflood.frag",
    .samplerCount = 1,
    .uniformBufferCount = 1,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, SDL_GPU_SAMPLECOUNT_1);
}
SDL_GPUGraphicsPipeline*
CreateVoronoiDistancePipeline()
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteLightingVoronoiDistance.frag",
    .samplerCount = 2,
    .uniformBufferCount = 1,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, SDL_GPU_SAMPLECOUNT_1);
}
SDL_GPUGraphicsPipeline*
CreateMixLightingAndScenePipeline(const SDL_GPUSampleCount sample_count = SDL_GPU_SAMPLECOUNT_1)
{
  const ShaderInput frag_input{
    .shaderFilename = "SpriteLightingMix.frag",
    .samplerCount = 2,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, pull_vert_input, frag_input, sample_count);
};

template<typename T>
SDL_GPUBuffer*
create_data_buffer(const int count)
{
  const auto buffer_info = SDL_GPUBufferCreateInfo{
    .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
    .size = static_cast<Uint32>(count * sizeof(T)),
  };
  auto* sprite_data_buffer = SDL_CreateGPUBuffer(device, &buffer_info);
  if (!sprite_data_buffer)
    throw SDLException("Unable to SDL_CreateGPUBuffer()");
  return sprite_data_buffer;
};

template<typename T>
SDL_GPUTransferBuffer*
create_transfer_buffer(const int count)
{
  const auto tb_info = SDL_GPUTransferBufferCreateInfo{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = static_cast<Uint32>(count * sizeof(T)),
  };
  auto* tb = SDL_CreateGPUTransferBuffer(device, &tb_info);
  if (!tb)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");
  return tb;
};

void
RenderThread()
{
  SDL_Log("%s", std::format("(RenderThread) SDL_IsMainThread(): {}", SDL_IsMainThread()).c_str());

  const uint32_t SPRITE_COUNT = 8192;

  RendererInfo renderer_info;
  renderer_info.device = device;
  renderer_info.window = window;
  setup_renderer(renderer_info);

  auto msaa = SDL_GPU_SAMPLECOUNT_1;
  const auto custom_texture_out = create_and_upload_gpu_texture(device, "textures/custom.png"s);

  auto* quad_data_transfer_buffer = create_transfer_buffer<SpriteInstance>(1); // fullscreen quad
  auto* quad_data_buffer = create_data_buffer<SpriteInstance>(1);

  auto* sprite_data_transfer_buffer = create_transfer_buffer<SpriteInstance>(SPRITE_COUNT);
  auto* sprite_data_buffer = create_data_buffer<SpriteInstance>(SPRITE_COUNT);

  auto* litsprite_data_transfer_buffer = create_transfer_buffer<SpriteInstance>(SPRITE_COUNT);
  auto* litsprite_data_buffer = create_data_buffer<SpriteInstance>(SPRITE_COUNT);

  const uint32_t MAX_LIGHTS = 32;
  auto* light_data_transfer_buffer = create_transfer_buffer<Light>(MAX_LIGHTS);
  auto* light_data_buffer = create_data_buffer<Light>(MAX_LIGHTS);

  Uint32 render_w = 0;
  Uint32 render_h = 0;
  // Uint32 render_w = 320;
  // Uint32 render_h = 240;
  render_w = (Uint32)(1.0f * window_w);
  render_h = (Uint32)(1.0f * window_h);

  // add padding to texture so light doesnt immediately dissapear
  const uint32_t light_padding = 50;
  const uint32_t light_w = window_w + (uint32_t)(2.0 * light_padding);
  const uint32_t light_h = window_h + (uint32_t)(2.0 * light_padding);

  const auto create_fixed_texture = [&](uint32_t w, uint32_t h) -> SDL_GPUTexture* {
    // The contents of this texture are undefined until data is written to the texture,
    // either via SDL_UploadToGpuTexture, or by performaing a render or compute pass with
    // this texture as a target.
    const SDL_GPUTextureCreateInfo gpu_texture_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GPU_TEXTUREFORMAT_R32G32_FLOAT,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = w,
      .height = h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };

    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device, &gpu_texture_info);
    if (gpu_texture == nullptr)
      throw SDLException("Unable to SDL_CreateGPUTexture()");
    return gpu_texture;
  };

  const auto create_render_texture = [&]() -> SDL_GPUTexture* {
    const SDL_GPUTextureCreateInfo gpu_texture_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = render_w,
      .height = render_h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device, &gpu_texture_info);
    if (gpu_texture == nullptr)
      throw SDLException("Unable to SDL_CreateGPUTexture()");
    return gpu_texture;
  };

  const auto create_msaa_texture = [&](SDL_GPUSampleCount in_msaa) -> SDL_GPUTexture* {
    // The contents of this texture are undefined until data is written to the texture,
    // either via SDL_UploadToGpuTexture, or by performaing a render or compute pass with
    // this texture as a target.
    SDL_GPUTextureCreateInfo gpu_texture_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
      .width = render_w,
      .height = render_h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = in_msaa,
    };

    if (in_msaa == SDL_GPU_SAMPLECOUNT_1)
      gpu_texture_info.usage |= SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device, &gpu_texture_info);
    if (gpu_texture == nullptr)
      throw SDLException("Unable to SDL_CreateGPUTexture()");
    return gpu_texture;
  };

  auto* sprite_pipeline = CreateSpritePipeline();
  auto* emitter_and_occluder_pipeline = CreateEmitterAndOccluderPipeline();
  auto* seed_pipeline = CreateLightingSeedPipeline();
  auto* jumpflood_pipeline = CreateJumpfloodPipeline();
  auto* voronoi_distance_pipeline = CreateVoronoiDistancePipeline();
  auto* mix_pipeline = CreateMixLightingAndScenePipeline();
  auto* error_pipeline = CreateErrorPipeline();

  // https://github.com/Turtwiggy/game_engine/blob/develop/game_defence/src/modules/core/renderer/system.cpp
  // engine::Shader lighting_emitters_and_occluders;
  // engine::Shader voronoi_seed; // this shader stores the uv coordinates in the texture
  // engine::Shader jump_flood;
  // engine::Shader voronoi_distance;

  auto* gpu_texture_a = create_render_texture();
  auto* gpu_texture_lighting_emitters_and_occluders = create_fixed_texture(light_w, light_h);
  auto* gpu_texture_lighting_voronoi_seed = create_fixed_texture(light_w, light_h);
  auto* gpu_texture_lighting_jump_flood_a = create_fixed_texture(light_w, light_h);
  auto* gpu_texture_lighting_jump_flood_b = create_fixed_texture(light_w, light_h);
  auto* gpu_texture_lighting_voronoi_distance = create_fixed_texture(light_w, light_h);

  SDL_Log("(RenderThread) -- done init");
  // SignalRenderThread(); // done init()
  // WaitForMainThread();

  tracy::SetThreadName("RenderThread");

  while (running) {
    ZoneScopedN("RenderThread");

    static Uint64 renderer_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, renderer_past);

    // handoff: game thread pushing data in to gameuidata
    // note: this doubles the memory because its duplicating RenderData
    auto& read_buffer = GetReadBuffer();
    {
      ZoneScopedN("(RenderThread) read_buffer_copy");
      std::unique_lock<std::mutex> lock0(read_buffer.mtx);
      std::unique_lock<std::mutex> lock1(game_ui_mtx);

      game_ui_data.renderable = read_buffer.renderable; // take a copy
      game_ui_data.ui_data = read_buffer.ui_data;
      game_ui_data.camera_pos = read_buffer.camera_pos;
      game_ui_data.lights = read_buffer.lights;
    }
    const auto& renderables = game_ui_data.renderable;
    const auto& camera_pos = game_ui_data.camera_pos;
    const auto& lights = game_ui_data.lights;

    // grab all the events
    //
    std::vector<SDL_Event> events;
    {
      ZoneScopedN("(RenderThread) events_dequeue_all()");
      events = rend_event_queue.dequeue_all();
    }

    // Rebuild pipelines & shaders
    {
      auto wants_to_rebuild_shaders = std::find_if(events.begin(), events.end(), [](const SDL_Event& e) {
                                        return e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_8;
                                      }) != events.end();
      wants_to_rebuild_shaders |= rebuild_shaders;
      if (wants_to_rebuild_shaders) {
        rebuild_shaders = false;

        // releasing and recreate the fill pipeline (which uses the updated shaders)
        SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, emitter_and_occluder_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, seed_pipeline);
        SDL_ReleaseGPUGraphicsPipeline(device, mix_pipeline);

        auto result = RecompileShaders();
        if (result != 0) {
          sprite_pipeline = CreateErrorPipeline();
          emitter_and_occluder_pipeline = CreateErrorPipeline();
          seed_pipeline = CreateErrorPipeline();
          jumpflood_pipeline = CreateErrorPipeline();
          mix_pipeline = CreateErrorPipeline();
        } else if (result == 0) {
          sprite_pipeline = CreateSpritePipeline();
          emitter_and_occluder_pipeline = CreateEmitterAndOccluderPipeline();
          seed_pipeline = CreateLightingSeedPipeline();
          jumpflood_pipeline = CreateJumpfloodPipeline();
          mix_pipeline = CreateMixLightingAndScenePipeline();
        }
      }
    }

    for (const auto& evt : events)
      ImGui_ImplSDL3_ProcessEvent(&evt);

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    // ImGui::ShowDemoWindow(NULL);

    {
      ImGui::Begin("Debug");

      if (rebuild_shaders)
        ImGui::Text("Rebuilding Shaders...");
      else if (ImGui::Button("Rebuild Shaders (8)"))
        rebuild_shaders = true;

      ImGui::Text("Rebuild DLL (9)");

      ImGui::End();

      const auto draw_tex = [](auto name, auto tex) {
        ImGui::Begin(name);
        const auto wh = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)tex, wh);
        ImGui::End();
      };
      draw_tex("GpuTextureA", gpu_texture_a);
      // draw_tex("Occluders", gpu_texture_lighting_emitters_and_occluders);
      // draw_tex("Seed", gpu_texture_lighting_voronoi_seed);
      // draw_tex("JumpfloodA", gpu_texture_lighting_jump_flood_a);
      // draw_tex("JumpfloodB", gpu_texture_lighting_jump_flood_b);
      // draw_tex("Distance", gpu_texture_lighting_voronoi_distance);
    }

    // Update game ui
    {
      if (game_code.valid) {
        ZoneScopedN("(RenderThread) game_update_ui()");
        game_code.game_update_ui(&game_ui_data);
      }
    }

    // SDL_Log("(RenderThread) Update()");
    {
      ImGui::Render();
      ImDrawData* draw_data = ImGui::GetDrawData();

      const float dt = (float)(1e-9 * (float)dt_ns);
      const auto camera_pos = game_ui_data.camera_pos;

      constexpr float M_PI = 3.141592653589f;
      const float aspect_ratio = (float)window_w / (float)window_h;

      // float zoom = 1.0f; // >1 = zoom in, <1 = zoom out
      // float half_w = (window_w / 2.0f) / zoom;
      // float half_h = (window_h / 2.0f) / zoom;
      // const auto proj_ortho_matrix = glm::ortho(-half_w, half_w, half_h, -half_w);

      const auto proj_ortho_light = glm::ortho(0.0f, (float)light_w, (float)light_h, 0.0f);
      const auto proj_ortho_matrix = glm::ortho(0.0f, (float)window_w, (float)window_h, 0.0f);
      const auto view_matrix = glm::translate(glm::mat4(1.0f), -camera_pos);

      const auto vp_matrix = proj_ortho_matrix * view_matrix;
      const auto vp_matrix_nopos = proj_ortho_matrix * glm::identity<glm::mat4>();
      const auto vp_matrix_light = proj_ortho_light * view_matrix;

      // "One can encode multiple render passes
      // (or alternate between render and compute passes) in a single command buffer.
      // Render passes can render to up to four color textures and one depth texture simultaneously."
      SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);
      if (cmd_buf == nullptr) {
        throw SDLException("Could not aquire GPU command buffer");
        exit(SDL_APP_FAILURE); // crash
      }

      // Sprites => GPU
      {
        // Build sprite instance transfer
        SpriteInstance* data_ptr = (SpriteInstance*)SDL_MapGPUTransferBuffer(device, sprite_data_transfer_buffer, true);
        for (Uint32 i = 0; i < SPRITE_COUNT; i += 1) {

          const bool draw = i < renderables.size();

          data_ptr[i].x = 0.0f;
          data_ptr[i].y = 0.0f;
          data_ptr[i].z = 0.0f;
          data_ptr[i].rotation = 0.0f;
          data_ptr[i].w = 0.0f;
          data_ptr[i].h = 0.0f;
          data_ptr[i].is_emitter = 0.0f;
          data_ptr[i].is_occluder = 0.0f;

          if (draw) {
            const auto& transform = renderables[i].transform;
            data_ptr[i].x = transform.pos.x;
            data_ptr[i].y = transform.pos.y;
            data_ptr[i].z = 0.0f;
            data_ptr[i].rotation = transform.rotation_radians;
            data_ptr[i].w = transform.size.x;
            data_ptr[i].h = transform.size.y;

            const auto& light_c = renderables[i].light;
            data_ptr[i].is_emitter = light_c.is_emitter;
            data_ptr[i].is_occluder = light_c.is_occluder;
          }

          data_ptr[i].tex_u = 0.0f;
          data_ptr[i].tex_v = 0.0f;
          data_ptr[i].tex_w = 1.0f;
          data_ptr[i].tex_h = 1.0f;

          if (draw) {
            const auto& colour = renderables[i].colour;
            data_ptr[i].colour[0] = colour.r;
            data_ptr[i].colour[1] = colour.g;
            data_ptr[i].colour[2] = colour.b;
            data_ptr[i].colour[3] = colour.a;

            auto& sprite_c = renderables[i].sprite;
            data_ptr[i].sprite_max_x = sprite_c.sprite_max_x;
            data_ptr[i].sprite_max_y = sprite_c.sprite_max_y;
            data_ptr[i].sprite_pos_x = sprite_c.sprite_pos_x;
            data_ptr[i].sprite_pos_y = sprite_c.sprite_pos_y;
            data_ptr[i].sprite_wh_x = sprite_c.sprite_wh_x;
            data_ptr[i].sprite_wh_y = sprite_c.sprite_wh_y;
          }

          data_ptr[i].p3 = 0.0f;
          data_ptr[i].p4 = 0.0f;
        }
        SDL_UnmapGPUTransferBuffer(device, sprite_data_transfer_buffer);

        // Upload instance data.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        {
          const auto transfer_buffer_loc = SDL_GPUTransferBufferLocation{
            .transfer_buffer = sprite_data_transfer_buffer,
            .offset = 0,
          };
          const auto gpu_buffer_region_loc = SDL_GPUBufferRegion{
            .buffer = sprite_data_buffer,
            .offset = 0,
            .size = SPRITE_COUNT * sizeof(SpriteInstance),
          };
          SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_loc, &gpu_buffer_region_loc, true);
        }
        SDL_EndGPUCopyPass(copy_pass);
      }

      // LitSprites => GPU
      {
        // Build sprite instance transfer
        SpriteInstance* data_ptr = (SpriteInstance*)SDL_MapGPUTransferBuffer(device, litsprite_data_transfer_buffer, true);
        for (Uint32 i = 0; i < SPRITE_COUNT; i += 1) {

          const bool draw = i < renderables.size();

          data_ptr[i].x = 0.0f;
          data_ptr[i].y = 0.0f;
          data_ptr[i].z = 0.0f;
          data_ptr[i].rotation = 0.0f;
          data_ptr[i].w = 0.0f;
          data_ptr[i].h = 0.0f;
          data_ptr[i].is_emitter = 0.0f;
          data_ptr[i].is_occluder = 0.0f;

          if (draw) {
            const auto& transform = renderables[i].transform;

            // Convert from worldspace to lightspace
            const auto worldspace_to_lightspace = [&](vec2 worldspace) -> vec2 {
              return { worldspace.x + light_padding, worldspace.y + light_padding };
            };
            const auto pos = worldspace_to_lightspace(transform.pos.xy());

            data_ptr[i].x = pos.x;
            data_ptr[i].y = pos.y;
            data_ptr[i].z = 0.0f;
            data_ptr[i].rotation = transform.rotation_radians;
            data_ptr[i].w = transform.size.x;
            data_ptr[i].h = transform.size.y;

            const auto& light_c = renderables[i].light;
            data_ptr[i].is_emitter = light_c.is_emitter;
            data_ptr[i].is_occluder = light_c.is_occluder;
          }

          data_ptr[i].tex_u = 0.0f;
          data_ptr[i].tex_v = 0.0f;
          data_ptr[i].tex_w = 1.0f;
          data_ptr[i].tex_h = 1.0f;

          if (draw) {
            const auto& colour = renderables[i].colour;
            data_ptr[i].colour[0] = colour.r;
            data_ptr[i].colour[1] = colour.g;
            data_ptr[i].colour[2] = colour.b;
            data_ptr[i].colour[3] = colour.a;

            auto& sprite_c = renderables[i].sprite;
            data_ptr[i].sprite_max_x = sprite_c.sprite_max_x;
            data_ptr[i].sprite_max_y = sprite_c.sprite_max_y;
            data_ptr[i].sprite_pos_x = sprite_c.sprite_pos_x;
            data_ptr[i].sprite_pos_y = sprite_c.sprite_pos_y;
            data_ptr[i].sprite_wh_x = sprite_c.sprite_wh_x;
            data_ptr[i].sprite_wh_y = sprite_c.sprite_wh_y;
          }

          data_ptr[i].p3 = 0.0f;
          data_ptr[i].p4 = 0.0f;
        }
        SDL_UnmapGPUTransferBuffer(device, litsprite_data_transfer_buffer);

        // Upload instance data.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        {
          const auto transfer_buffer_loc = SDL_GPUTransferBufferLocation{
            .transfer_buffer = litsprite_data_transfer_buffer,
            .offset = 0,
          };
          const auto gpu_buffer_region_loc = SDL_GPUBufferRegion{
            .buffer = litsprite_data_buffer,
            .offset = 0,
            .size = SPRITE_COUNT * sizeof(SpriteInstance),
          };
          SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_loc, &gpu_buffer_region_loc, true);
        }
        SDL_EndGPUCopyPass(copy_pass);
      }

      // Upload exactly 1 quad to the quad_data_buffer.
      {
        // Build sprite instance transfer
        SpriteInstance* data_ptr = (SpriteInstance*)SDL_MapGPUTransferBuffer(device, quad_data_transfer_buffer, true);
        data_ptr[0].x = 0.0f;
        data_ptr[0].y = 0.0f;
        data_ptr[0].z = 0.0f;
        data_ptr[0].rotation = 0.0f;
        data_ptr[0].w = (float)window_w;
        data_ptr[0].h = (float)window_h;
        data_ptr[0].is_emitter = 0.0f;
        data_ptr[0].is_occluder = 0.0f;
        data_ptr[0].tex_u = 0.0f;
        data_ptr[0].tex_v = 0.0f;
        data_ptr[0].tex_w = 1.0f;
        data_ptr[0].tex_h = 1.0f;
        data_ptr[0].colour[0] = 1.0f;
        data_ptr[0].colour[1] = 0.0f;
        data_ptr[0].colour[2] = 0.0f;
        data_ptr[0].colour[3] = 1.0f;
        data_ptr[0].sprite_max_x = 0;
        data_ptr[0].sprite_max_y = 0;
        data_ptr[0].sprite_pos_x = 0;
        data_ptr[0].sprite_pos_y = 0;
        data_ptr[0].sprite_wh_x = 0;
        data_ptr[0].sprite_wh_y = 0;
        data_ptr[0].p3 = 0.0f;
        data_ptr[0].p4 = 0.0f;
        SDL_UnmapGPUTransferBuffer(device, quad_data_transfer_buffer);

        // Upload instance data.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        {
          const auto transfer_buffer_loc = SDL_GPUTransferBufferLocation{
            .transfer_buffer = quad_data_transfer_buffer,
            .offset = 0,
          };
          const auto gpu_buffer_region_loc = SDL_GPUBufferRegion{
            .buffer = quad_data_buffer,
            .offset = 0,
            .size = 1 * sizeof(SpriteInstance),
          };
          SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_loc, &gpu_buffer_region_loc, true);
        }
        SDL_EndGPUCopyPass(copy_pass);
      }

      // Lights StructuredBuffer => GPU
      {
        Light* data_ptr = (Light*)SDL_MapGPUTransferBuffer(device, light_data_transfer_buffer, true);
        for (Uint32 i = 0; i < MAX_LIGHTS; i++) {

          data_ptr[i].pos_x = 0;
          data_ptr[i].pos_y = 0;
          data_ptr[i].pos_z = 0;
          data_ptr[i].enabled = 0;

          const bool draw = i < lights.size();
          if (draw) {
            data_ptr[i].pos_x = lights[i].pos_x;
            data_ptr[i].pos_y = lights[i].pos_y;
            data_ptr[i].pos_z = lights[i].pos_z;
            data_ptr[i].enabled = 1.0f;
          }

          //
        }
        SDL_UnmapGPUTransferBuffer(device, light_data_transfer_buffer);

        // Upload instance data.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        {
          const auto transfer_buffer_loc = SDL_GPUTransferBufferLocation{
            .transfer_buffer = light_data_transfer_buffer,
            .offset = 0,
          };
          const auto gpu_buffer_region_loc = SDL_GPUBufferRegion{
            .buffer = light_data_buffer,
            .offset = 0,
            .size = MAX_LIGHTS * sizeof(Light),
          };
          SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_loc, &gpu_buffer_region_loc, true);
        }
        SDL_EndGPUCopyPass(copy_pass);
      }

      // render the sprites to a texture
      render_to_texture(
        //
        cmd_buf,
        sprite_pipeline,
        sprite_data_buffer,
        gpu_texture_a,
        renderer_info.samplers[0],
        vp_matrix,
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { custom_texture_out.texture },
        SPRITE_COUNT,
        nullptr);

      // render emitters and occluders
      render_to_texture(
        //
        cmd_buf,
        emitter_and_occluder_pipeline,
        litsprite_data_buffer,
        gpu_texture_lighting_emitters_and_occluders,
        renderer_info.samplers[0],
        vp_matrix_light,
        { 0.0f, 0.0f, 0.0f, 0.0f },
        {},
        SPRITE_COUNT,
        nullptr);

      // Lighting: The Seed
      render_to_texture(
        //
        cmd_buf,
        seed_pipeline,
        quad_data_buffer,
        gpu_texture_lighting_voronoi_seed,
        renderer_info.samplers[0],
        vp_matrix_nopos,
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { gpu_texture_lighting_emitters_and_occluders },
        1,
        nullptr);

      // Lighting: Jumpflood
      const int max_dim = glm::max(light_w, light_h);
      const int n_jumpflood_passes = (int)(glm::ceil(glm::log(max_dim) / std::log(2.0f)));
      SDL_GPUTexture* final_lighting_texture = nullptr;
      for (int i = 0; i < n_jumpflood_passes; i++) {

        const float offset = (float)std::pow(2, n_jumpflood_passes - i - 1);

        SDL_GPUTexture* texture_to_sample_from = gpu_texture_lighting_voronoi_seed;
        SDL_GPUTexture* texture_to_render_to = gpu_texture_lighting_jump_flood_a;

        if (i > 0) {
          const int this_tex_idx = (i + 1) % 2;
          texture_to_sample_from = this_tex_idx == 0 ? gpu_texture_lighting_jump_flood_a : gpu_texture_lighting_jump_flood_b;
          texture_to_render_to = this_tex_idx == 0 ? gpu_texture_lighting_jump_flood_b : gpu_texture_lighting_jump_flood_a;

          // if (this_tex_idx == 0)
          //   SDL_Log("i: %d, this_tex_idx: %d (config: a, b)", i, this_tex_idx);
          // else
          //   SDL_Log("i: %d, this_tex_idx: %d (config: b, a)", i, this_tex_idx);
        }

        UniformBlock jumpflood_ubo;
        jumpflood_ubo.data[0] = offset;
        jumpflood_ubo.data[1] = light_w;
        jumpflood_ubo.data[2] = light_h;
        jumpflood_ubo.data[3] = 0.0f;
        render_to_texture(
          //
          cmd_buf,
          jumpflood_pipeline,
          quad_data_buffer,
          texture_to_render_to,
          renderer_info.samplers[0],
          vp_matrix_nopos,
          { 0.0f, 0.0f, 0.0f, 1.0f },
          { texture_to_sample_from },
          1,
          &jumpflood_ubo);

        final_lighting_texture = texture_to_render_to;
      }

      // Lighting: Voronoi Distance

      UniformBlock voronoi_distance_ubo;
      voronoi_distance_ubo.data[0] = render_w;
      voronoi_distance_ubo.data[1] = render_h;
      voronoi_distance_ubo.data[2] = 0.0f;
      voronoi_distance_ubo.data[3] = 0.0f;
      render_to_texture(cmd_buf,
                        voronoi_distance_pipeline,
                        quad_data_buffer,
                        gpu_texture_lighting_voronoi_distance,
                        renderer_info.samplers[0],
                        vp_matrix_nopos,
                        { 0.0f, 0.0f, 0.0f, 1.0f },
                        { final_lighting_texture, gpu_texture_lighting_emitters_and_occluders },
                        1,
                        &voronoi_distance_ubo);

      // Output
      UniformBlock ubo_final;
      ubo_final.data[0] = (float)render_w;
      ubo_final.data[1] = (float)render_h;
      ubo_final.data[2] = light_padding;
      ubo_final.data[3] = 0.0f;

      render_to_swapchain(
        //
        cmd_buf,
        mix_pipeline,
        quad_data_buffer,
        light_data_buffer,
        {
          gpu_texture_a,
          gpu_texture_lighting_voronoi_distance,
          // gpu_texture_lighting_emitters_and_occluders,
        },
        renderer_info.samplers[0],
        window,
        draw_data,
        vp_matrix_nopos,
        ubo_final);

      // Update and Render additional Platform Windows
      // clang-format off
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      // clang-format on
      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
      }

      const auto submit = SDL_SubmitGPUCommandBuffer(cmd_buf);
      if (!submit)
        throw SDLException("Could not SDL_SubmitGPUCommandBuffer()");
    }

    FrameMark; // frame done
  }

  SDL_Log("(RenderThread) shutting down...");
  cleanup_imgui(device);
  SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
  SDL_ReleaseGPUGraphicsPipeline(device, error_pipeline);
  if (custom_texture_out.texture != nullptr)
    SDL_ReleaseGPUTexture(device, custom_texture_out.texture);
  SDL_ReleaseGPUTexture(device, gpu_texture_a);
  for (int i = 0; i < SDL_arraysize(renderer_info.samplers); i++) {
    if (renderer_info.samplers[i] != nullptr)
      SDL_ReleaseGPUSampler(device, renderer_info.samplers[i]);
  }
  SDL_ReleaseGPUTransferBuffer(device, sprite_data_transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, quad_data_transfer_buffer);
  SDL_ReleaseGPUBuffer(device, sprite_data_buffer);
  SDL_ReleaseGPUBuffer(device, quad_data_buffer);
};

//
// General approach
//
// write the events to a thread-safe input buffer.
// share this buffer with the game-thread.
//
// Game-thread grabs all input & processes.
// Update physics, gamelogic, etc,
// the state is written to a game-state-buffer
//
// Render-Thread reads the most recent game-state and renders it.
// It does not wait
// for the game thread to finish the next frame,
// it renders the latest available state.
//

int
main(int argc, char* argv[])
{
  // setvbuf(stdout, nullptr, _IONBF, 0); // dont buffer

  if (!SDL_SetAppMetadata("SomeCoolGame", "1.0", "com.blueberrygames.game"))
    throw SDLException("Couldn't SDL_SetAppMetadata()");
  if (!SDL_Init(SDL_INIT_VIDEO))
    throw SDLException("Failed to SDL_Init(SDL_INIT_VIDEO)");
  if (!SDL_Init(SDL_INIT_JOYSTICK))
    throw SDLException("Failed to SDL_Init(SDL_INIT_JOYSTICK)");

  const float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
  setup_sdl();
  setup_sdl_controllers();
  device = setup_sdl_gpu();
  window = setup_sdl_window(main_scale, window_w, window_h);

  if (!SDL_ClaimWindowForGPUDevice(device, window))
    throw SDLException("Couldn't claim window for GPU device");

  auto* device_driver = SDL_GetGPUDeviceDriver(device);
  if (!device_driver)
    throw SDLException("Couldn't get GPU device driver");
  SDL_Log("Using GPU device driver: %s", device_driver);

  ImGuiSetup im_setup;
  im_setup.game_ui_data = &game_ui_data;
  im_setup.main_scale = main_scale;
  im_setup.window = window;
  im_setup.device = device;
  setup_imgui(im_setup);

  int result = RecompileShaders();
  if (result != 0) {
    throw std::exception("Failed to compile shaders.");
    exit(SDL_APP_FAILURE);
  }

  // #elif __linux__
  //   "libGameDLL.so";
  // #elif __APPLE__
  //   "libGameDLL.dylib";
  // Load GameDLL.dll on launch
  const auto src_dll = "GameDLL-hot-unlocked.dll";
  const auto dst_dll = "GameDLL-hot-locked.dll"; // when loaded, system processor locks it
  static std::atomic_bool rebuild_dll(false);
  delete_hotreload_locked_dll();
  sdl_load_game_code(game_code, src_dll, dst_dll);

  // Start threads, innit
  std::thread game_thread(GameThread);
  std::thread render_thread(RenderThread);

  while (running) {
    ZoneScopedN("MainThread");
    static Uint64 past = SDL_GetTicksNS();
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, past);
    const Uint64 start_s = SDL_GetPerformanceCounter();

    // SDL_PollEvent: MainThread
    static std::vector<SDL_Event> evts;
    evts.clear();
    {
      ZoneScopedN("(MainThread) poll_events()");

      SDL_Event evt;
      while (SDL_PollEvent(&evt)) {

        if (evt.type == SDL_EVENT_QUIT)
          running = false;
        if (evt.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && evt.window.windowID == SDL_GetWindowID(window))
          running = false;

        if (evt.type == SDL_EVENT_KEY_DOWN) {
          const auto scancode = evt.key.scancode;
          if (scancode == SDL_SCANCODE_9) {
            SDL_Log("(KEY: 9) Rebuild dll...");
            rebuild_dll = true;
          }
          if (scancode == SDL_SCANCODE_ESCAPE)
            running = false;
        }

        evts.push_back(evt);
      }
    }

    // push all the events at once in to a thread-safe vector.
    game_event_queue.enqueue(evts);
    rend_event_queue.enqueue(evts);

    // Call SDL_GetMouseState on main thread.
    SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);

    if (rebuild_dll) {
      do_rebuild_dll(game_code, src_dll, dst_dll);
      rebuild_dll = false;
    }

    SDL_Delay(1); // slow down the event thread
    FrameMark;    // frame done
  }

  game_thread.join();
  render_thread.join();

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);
  SDL_Quit();

  return 0;
};
