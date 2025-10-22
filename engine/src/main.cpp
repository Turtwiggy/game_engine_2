#include "pch.hpp"

// #include "box2d_parallel.hpp"
#include "game_and_engine_interop.hpp"
#include "imgui_helpers.hpp"
#include "modules/camera/perspective_helpers.hpp"
#include "modules/glm/glm_helpers.hpp"
#include "modules/models/model_helpers.hpp"
#include "modules/models_animations/model_animation_components.hpp"
#include "modules/models_animations/model_animation_helpers.hpp"
#include "modules/renderer/renderer_helpers.hpp"
#include "modules/sdl/sdl_exception.hpp"
#include "modules/sdl/sdl_hot_reload_dll.hpp"
#include "modules/sdl/sdl_setup.hpp"
#include "modules/sdl/sdl_shader.hpp"
#include "threadsafe_queue.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

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
static int window_w = 1280, window_h = 720;

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

const auto calc_dt_ns = [](Uint64 now, Uint64& past) -> Uint64 {
  Uint64 dt_ns = now - past;
  dt_ns = std::min(dt_ns, Uint64(250 * 1e6)); // avoid spiral
  past = now;
  return dt_ns;
};

void
GameThread()
{
  const auto info_str = std::format("(GameThread) SDL_IsMainThread(): {}", SDL_IsMainThread());
  SDL_Log("%s", info_str.c_str());

  if (!game_code.valid) {
    throw std::runtime_error("Failed to load .dll");
    exit(SDL_APP_FAILURE);
  }

  // physics innit
  const auto logical_cpu_cores = SDL_GetNumLogicalCPUCores();
  SDL_Log("LogicalCPUCores: %i", logical_cpu_cores);

  // threads used (main, game, render)
  // const int used_threads = 3;
  // const int max_thread_count = std::max(1, logical_cpu_cores - used_threads);
  // SDL_Log("%s", std::format("Giving box2d {} threads", max_thread_count).c_str());
  // TODO: work out what this value should be
  // int worker_count = 1;
  // std::shared_ptr<Sample> s = std::make_shared<Sample>();
  // s->m_scheduler.Initialize(worker_count);
  // s->m_taskCount = 0;

  //  game init after physics init
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
      game_code.rebuilt = false;
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

      // b2World_Step(game_data.world_id, physics_dt, physics_substep_count);
      // s->m_taskCount = 0;
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
      wb.ui_data.hmm.clear();

      if (game_code.valid) {
        // copy transforms in to RenderData.
        const auto view = game_data.r->view<const TransformComponent, const ColourComponent, const SpriteComponent>();
        view.each([&](entt::entity e, const auto& t_c, const auto& col_c, const auto& sprite_c) {
          wb.renderable.push_back(Renderable{
            .transform = t_c,
            .colour = col_c,
            .sprite = sprite_c,
          });
        });

        // copy anything else in to renderdata buffer.
        wb.camera_c = game_data.camera_c;
        wb.camera_pos = game_data.camera_pos;
        wb.ui_data = game_data.ui_data;
        wb.ui_data.game_dt = dt;
      }
    }

    SwapBuffers();
    FrameMark; // frame done
  }

  b2DestroyWorld(game_data.world_id);
};

typedef struct SpriteInstance
{
  float x, y, z;
  float rotation;
  float w, h, p1, p2;
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

SDL_GPUGraphicsPipeline*
CreateErrorPipeline()
{
  const ShaderInput vert_input{
    .shaderFilename = "PullSpriteBatch.vert",
    .samplerCount = 0,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  const ShaderInput frag_input{
    .shaderFilename = "SolidColorInput.frag",
    .samplerCount = 0,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, vert_input, frag_input);
};

SDL_GPUGraphicsPipeline*
CreateSpritePipeline()
{
  const ShaderInput vert_input{
    .shaderFilename = "PullSpriteBatch.vert",
    .samplerCount = 0,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  const ShaderInput frag_input{
    .shaderFilename = "SpriteSheet.frag",
    .samplerCount = 1,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, vert_input, frag_input);
};
SDL_GPUGraphicsPipeline*
CreateSpriteNormalPipeline()
{
  const ShaderInput vert_input{
    .shaderFilename = "PullSpriteBatch.vert",
    .samplerCount = 0,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  const ShaderInput frag_input{
    .shaderFilename = "SpriteSheetNormals.frag",
    .samplerCount = 1,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, vert_input, frag_input);
};
SDL_GPUGraphicsPipeline*
CreateLightingPipeline()
{
  const ShaderInput vert_input{
    .shaderFilename = "PullSpriteBatch.vert",
    .samplerCount = 0,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  const ShaderInput frag_input{
    .shaderFilename = "Lighting.frag",
    .samplerCount = 2,
    .uniformBufferCount = 1,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_2d_pipeline(device, window, vert_input, frag_input);
};
SDL_GPUGraphicsPipeline*
CreateModelPipeline(const SDL_GPUSampleCount sample_count)
{
  const ShaderInput vert_input{
    .shaderFilename = "ModelAnimated.vert",
    .samplerCount = 0,
    .uniformBufferCount = 1,
    .storageBufferCount = 1,
    .storageTextureCount = 0,
  };
  const ShaderInput frag_input{
    .shaderFilename = "ModelAnimated.frag",
    .samplerCount = 0,
    .uniformBufferCount = 0,
    .storageBufferCount = 0,
    .storageTextureCount = 0,
  };
  return create_model_pipeline(device, window, vert_input, frag_input, sample_count);
};

struct BoneData
{
  glm::mat4 matrix = glm::mat4(1.0f);
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

  static SINGLE_ModelsComponent models;
  static SINGLE_AnimatorComponent anims;
  {
    models.model_0_data = load_model(device, "assets/models/wiggy_mech_2d4b.fbx"s);
    anims.animation_0_data = load_animation(models.model_0_data);
    load_animations(anims, models);
  }

  auto msaa = SDL_GPU_SAMPLECOUNT_1;
  // const auto viking_texture = create_and_upload_gpu_texture(device, "models/viking_room.png"s);
  const auto custom_texture_out = create_and_upload_gpu_texture(device, "textures/custom.png"s);
  const auto custom_texture_normal_out = create_and_upload_gpu_texture(device, "textures/custom_normal.png"s);
  auto* sprite_pipeline = CreateSpritePipeline();
  auto* normal_pipeline = CreateSpriteNormalPipeline();
  auto* lighting_pipeline = CreateLightingPipeline();
  auto* error_pipeline = CreateErrorPipeline();
  auto* model_pipeline = CreateModelPipeline(msaa);

  auto* sprite_data_transfer_buffer = create_transfer_buffer<SpriteInstance>(SPRITE_COUNT);
  auto* sprite_data_buffer = create_data_buffer<SpriteInstance>(SPRITE_COUNT);

  auto* quad_data_transfer_buffer = create_transfer_buffer<SpriteInstance>(1); // fullscreen quad
  auto* quad_data_buffer = create_data_buffer<SpriteInstance>(1);

  auto* bone_data_transfer_buffer = create_transfer_buffer<BoneData>(anims.amount_of_bones);
  auto* bone_data_buffer = create_data_buffer<BoneData>(anims.amount_of_bones);
  assert(sizeof(BoneData) == sizeof(glm::mat4));

  const auto create_render_texture = []() -> SDL_GPUTexture* {
    // The contents of this texture are undefined until data is written to the texture,
    // either via SDL_UploadToGpuTexture, or by performaing a render or compute pass with
    // this texture as a target.
    const SDL_GPUTextureCreateInfo gpu_texture_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = (Uint32)2.0f * window_w,
      .height = (Uint32)2.0f * window_h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };
    SDL_GPUTexture* gpu_texture = SDL_CreateGPUTexture(device, &gpu_texture_info);
    if (gpu_texture == nullptr)
      throw SDLException("Unable to SDL_CreateGPUTexture()");
    return gpu_texture;
  };
  auto* gpu_texture_a = create_render_texture();
  auto* gpu_texture_b = create_render_texture();
  auto* gpu_texture_c = create_render_texture();

  auto create_msaa_texture = [](auto in_msaa) -> SDL_GPUTexture* {
    // The contents of this texture are undefined until data is written to the texture,
    // either via SDL_UploadToGpuTexture, or by performaing a render or compute pass with
    // this texture as a target.
    SDL_GPUTextureCreateInfo gpu_texture_info = {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = SDL_GetGPUSwapchainTextureFormat(device, window),
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
      .width = (Uint32)2.0f * window_w,
      .height = (Uint32)2.0f * window_h,
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
  auto* msaa_texture = create_msaa_texture(msaa);
  auto* msaa_depth_texture = create_depth_texture(device, 2.0f * window_w, 2.0f * window_h, msaa);

  SDL_Log("(RenderThread) -- done init");
  // SignalRenderThread(); // done init()
  // WaitForMainThread();

  tracy::SetThreadName("RenderThread");

  while (running) {
    ZoneScopedN("RenderThread");

    static Uint64 renderer_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, renderer_past);

    // handoff: game thread pusning data in to gameuidata
    // note: this doubles the memory because its duplicating RenderData
    auto& read_buffer = GetReadBuffer();
    {
      ZoneScopedN("(RenderThread) read_buffer_copy");
      std::unique_lock<std::mutex> lock0(read_buffer.mtx);
      std::unique_lock<std::mutex> lock1(game_ui_mtx);

      game_ui_data.renderable = read_buffer.renderable; // take a copy
      game_ui_data.ui_data = read_buffer.ui_data;
      game_ui_data.camera_c = read_buffer.camera_c;
      game_ui_data.camera_pos = read_buffer.camera_pos;
    }
    const auto& renderables = game_ui_data.renderable;
    const auto& camera_c = game_ui_data.camera_c;
    const auto& camera_pos = game_ui_data.camera_pos;

    //
    // grab all the events
    //
    std::vector<SDL_Event> events;
    {
      ZoneScopedN("(RenderThread) events_dequeue_all()");
      events = rend_event_queue.dequeue_all();
    }

    // check if a key was pressed.
    {
      ZoneScopedN("(RenderThread) events");
      for (const auto& evt : events) {
        if (evt.type == SDL_EVENT_KEY_DOWN) {
          const auto scancode = evt.key.scancode;

          if (scancode == SDL_SCANCODE_8) {
            SDL_Log("(RenderThread) wants to rebuild shaders...");

            // releasing and recreate the fill pipeline (which uses the updated shaders)
            SDL_ReleaseGPUGraphicsPipeline(device, model_pipeline);
            SDL_ReleaseGPUGraphicsPipeline(device, sprite_pipeline);
            SDL_ReleaseGPUGraphicsPipeline(device, normal_pipeline);
            SDL_ReleaseGPUGraphicsPipeline(device, lighting_pipeline);

            auto result = RecompileShaders();
            if (result != 0) {
              model_pipeline = CreateErrorPipeline();
              sprite_pipeline = CreateErrorPipeline();
              normal_pipeline = CreateErrorPipeline();
              lighting_pipeline = CreateErrorPipeline();
            } else if (result == 0) {
              model_pipeline = CreateModelPipeline(msaa);
              sprite_pipeline = CreateSpritePipeline();
              normal_pipeline = CreateSpriteNormalPipeline();
              lighting_pipeline = CreateLightingPipeline();
            }
          }
        }
        if (evt.type == SDL_EVENT_WINDOW_RESIZED) {
          SDL_Log("TODO: recreate render/depth textures");
          // SDL_ReleaseGPUTexture(device, gpu_texture_a);
          // SDL_ReleaseGPUTexture(device, gpu_texture_b);
          // SDL_ReleaseGPUTexture(device, gpu_texture_c);
          // SDL_ReleaseGPUTexture(device, msaa_depth_texture);
          // SDL_GetWindowSize(window, &window_w, &window_h);
          // msaa_depth_texture = create_depth_texture(device, 2.0f * window_w, 2.0f * window_h, msaa);
          // gpu_texture_a = create_render_texture();
          // gpu_texture_b = create_render_texture();
          // gpu_texture_c = create_render_texture();
        }
      }
    }

    for (const auto& evt : events)
      ImGui_ImplSDL3_ProcessEvent(&evt);

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(NULL);

    {
      ImGui::Begin("Debug");

      static int msaa_factor = 2;
      ImGui::InputInt("msaa", &msaa_factor);
      if (ImGui::Button("Refresh Msaa")) {

        if (msaa_factor == 1)
          msaa = SDL_GPU_SAMPLECOUNT_1;
        if (msaa_factor == 2)
          msaa = SDL_GPU_SAMPLECOUNT_2;
        if (msaa_factor == 4)
          msaa = SDL_GPU_SAMPLECOUNT_4;
        if (msaa_factor == 8)
          msaa = SDL_GPU_SAMPLECOUNT_8;

        // recreate textures
        SDL_ReleaseGPUTexture(device, msaa_texture);
        SDL_ReleaseGPUTexture(device, msaa_depth_texture);
        msaa_texture = create_msaa_texture(msaa);
        msaa_depth_texture = create_depth_texture(device, 2.0f * window_w, 2.0f * window_h, msaa);

        // recreate pipeline.
        SDL_ReleaseGPUGraphicsPipeline(device, model_pipeline);
        model_pipeline = CreateModelPipeline(msaa);
      }

      ImGui::Text("anim: %f", anims.current_time);
      if (ImGui::Button("StartAnim"))
        anims.current_animation = &anims.animation_0_data;
      if (ImGui::Button("StopAnim"))
        anims.current_animation = nullptr;

      ImGui::End();

      ImGui::Begin("GpuTextureA");
      {
        const auto wh = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)gpu_texture_a, wh);
      }
      ImGui::End();
      ImGui::Begin("GpuTextureB");
      {
        const auto wh = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)gpu_texture_b, wh);
      }
      ImGui::End();
      ImGui::Begin("GpuTextureC");
      {
        const auto wh = ImGui::GetContentRegionAvail();
        ImGui::Image((ImTextureID)(intptr_t)gpu_texture_c, wh);
      }
      ImGui::End();
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
      const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

      const float dt = (float)(1e-9 * (float)dt_ns);
      constexpr float M_PI = 3.141592653589;
      const float aspect_ratio = (float)window_w / (float)window_h;
      const auto proj_ortho_matrix = glm::ortho(0.0f, (float)window_w, (float)window_h, 0.0f);
      const auto view_matrix = glm::lookAt(glm::vec3{ 0.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
      const auto vp_matrix = proj_ortho_matrix * view_matrix;

      // this should probaby be in the game() loop
      update_animation(anims, dt);

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

          if (draw) {
            const auto& transform = renderables[i].transform;
            data_ptr[i].x = transform.pos.x;
            data_ptr[i].y = transform.pos.y;
            data_ptr[i].z = 0.0f;
            data_ptr[i].rotation = transform.rotation_radians;
            data_ptr[i].w = transform.size.x;
            data_ptr[i].h = transform.size.y;
          }

          data_ptr[i].p1 = 0.0f;
          data_ptr[i].p2 = 0.0f;
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
        data_ptr[0].p1 = 0.0f;
        data_ptr[0].p2 = 0.0f;
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

      // Upload bone data matricies.
      {
        const auto& final_bone_matrices = anims.final_bone_matrices;
        const auto matrices_size = final_bone_matrices.size();

        BoneData* data_ptr = (BoneData*)SDL_MapGPUTransferBuffer(device, bone_data_transfer_buffer, true);
        for (int i = 0; i < anims.amount_of_bones; i++) {
          // upload the data.
          if (i < matrices_size)
            data_ptr[i].matrix = final_bone_matrices[i];
          else
            data_ptr[i].matrix = glm::mat4(1.0f);
        }
        SDL_UnmapGPUTransferBuffer(device, bone_data_transfer_buffer);

        // Upoad instance data.
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
        {
          const auto transfer_buffer_loc = SDL_GPUTransferBufferLocation{
            .transfer_buffer = bone_data_transfer_buffer,
            .offset = 0,
          };
          const auto gpu_buffer_region_loc = SDL_GPUBufferRegion{
            .buffer = bone_data_buffer,
            .offset = 0,
            .size = (uint32_t)(final_bone_matrices.size() * sizeof(BoneData)),
          };
          SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_loc, &gpu_buffer_region_loc, true);
        }
        SDL_EndGPUCopyPass(copy_pass);
      }

      // render the sprites to a texture
      {
        const SDL_GPUColorTargetInfo col_info_a = {
          .texture = gpu_texture_a,
          .mip_level = 0,
          .layer_or_depth_plane = 0,
          .clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f },
          .load_op = SDL_GPU_LOADOP_CLEAR,
          .store_op = SDL_GPU_STOREOP_STORE,
          .cycle = false,
        };
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info_a, 1, NULL);
        {
          SDL_BindGPUGraphicsPipeline(render_pass, sprite_pipeline);
          SDL_BindGPUVertexStorageBuffers(render_pass, 0, &sprite_data_buffer, 1);
          SDL_PushGPUVertexUniformData(cmd_buf, 0, &vp_matrix, sizeof(glm::mat4));

          const auto fragment_samplers = std::vector<SDL_GPUTextureSamplerBinding>{
            { .texture = custom_texture_out.texture, .sampler = renderer_info.samplers[0] },
          };
          SDL_BindGPUFragmentSamplers(render_pass, 0, fragment_samplers.data(), (uint32_t)fragment_samplers.size());

          SDL_DrawGPUPrimitives(render_pass, SPRITE_COUNT * 6, 1, 0, 0);
        }
        SDL_EndGPURenderPass(render_pass);
      }

      // render the sprite normals to a texture
      {
        const SDL_GPUColorTargetInfo col_info_a = {
          .texture = gpu_texture_b,
          .mip_level = 0,
          .layer_or_depth_plane = 0,
          .clear_color = SDL_FColor{ 0.0f, 0.0f, 0.0f, 1.0f },
          .load_op = SDL_GPU_LOADOP_CLEAR,
          .store_op = SDL_GPU_STOREOP_STORE,
          .cycle = false,
        };
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info_a, 1, NULL);
        {
          SDL_BindGPUGraphicsPipeline(render_pass, normal_pipeline);
          SDL_BindGPUVertexStorageBuffers(render_pass, 0, &sprite_data_buffer, 1);
          SDL_PushGPUVertexUniformData(cmd_buf, 0, &vp_matrix, sizeof(glm::mat4));

          const auto fragment_samplers = std::vector<SDL_GPUTextureSamplerBinding>{
            { .texture = custom_texture_normal_out.texture, .sampler = renderer_info.samplers[0] },
          };
          SDL_BindGPUFragmentSamplers(render_pass, 0, fragment_samplers.data(), (uint32_t)fragment_samplers.size());

          SDL_DrawGPUPrimitives(render_pass, SPRITE_COUNT * 6, 1, 0, 0);
        }
        SDL_EndGPURenderPass(render_pass);
      }

      // render an animated model
      {
        const SDL_GPUDepthStencilTargetInfo depth_info_c = {
          .texture = msaa_depth_texture,
          .clear_depth = 1.0f,
          .load_op = SDL_GPU_LOADOP_CLEAR,
        };
        SDL_GPUColorTargetInfo col_info_c{
          .mip_level = 0,
          .layer_or_depth_plane = 0,
          .clear_color = SDL_FColor{ 0.3f, 0.3f, 0.3f, 1.0f },
          .load_op = SDL_GPU_LOADOP_CLEAR,
          .cycle = false,
        };
        if (msaa != SDL_GPU_SAMPLECOUNT_1) {
          col_info_c.texture = msaa_texture;
          col_info_c.resolve_texture = gpu_texture_c; // resolve_texture must have sample_count of 1
          col_info_c.store_op = SDL_GPU_STOREOP_RESOLVE;
        } else {
          col_info_c.texture = gpu_texture_c;
          col_info_c.store_op = SDL_GPU_STOREOP_STORE;
        }

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info_c, 1, &depth_info_c);
        {
          SDL_BindGPUGraphicsPipeline(render_pass, model_pipeline);
          SDL_BindGPUVertexStorageBuffers(render_pass, 0, &bone_data_buffer, 1);

          const auto persp_view = camera_c.view;
          const auto persp_proj = camera_c.projection;

          glm::mat4 model_matrix(1.0f);
          // model_matrix = glm::scale(model_matrix, t.scale);
          // model_matrix = Matrix4x4_Rotate(model_matrix, radians(t) * 0.01f, vec3{ 0.0f, 1.0f, 0.0f });
          // model = glm::translate(model, t.position);
          // model = glm::scale(model, t.scale);
          // model_matrix *= glm::toMat4(vec3_to_quat({ glm::radians(90.0f), 0, 0 }));
          // // draw_model(models_c.models_to_load[0]);
          // static float t = 0.0f;
          // t += dt;
          // model_matrix = glm::rotate(model_matrix, glm::radians(t) * 10.0f, { 0.0f, 1.0f, 0.0f });
          model_matrix = glm::scale(model_matrix, { 0.01, 0.01, 0.01 });

          const auto mvp_matrix = persp_proj * persp_view * model_matrix;
          SDL_PushGPUVertexUniformData(cmd_buf, 0, &mvp_matrix, sizeof(glm::mat4));

          const auto& model = models.model_0_data;
          for (const auto& mesh : model.meshes) {

            auto* vertex_buffer = mesh.vertex_buffer;
            auto* index_buffer = mesh.index_buffer;
            auto index_buffer_size = mesh.index_data.size();

            SDL_GPUBufferBinding vertex_binding{ vertex_buffer, 0 };
            SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

            SDL_GPUBufferBinding index_binding{ index_buffer, 0 };
            SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            SDL_DrawGPUIndexedPrimitives(render_pass, (uint32_t)index_buffer_size, 1, 0, 0, 0);
          }
        }
        SDL_EndGPURenderPass(render_pass);
      }

      // https://wiki.libsdl.org/SDL3/SDL_WaitAndAcquireGPUSwapchainTexture
      SDL_GPUTexture* swapchain_texture;
      SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_texture, nullptr, nullptr);
      if (swapchain_texture != nullptr && !is_minimized) {

        // This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

        //
        // Render the final output to the swapchain_texture
        //
        {
          SDL_PushGPUVertexUniformData(cmd_buf, 0, &vp_matrix, sizeof(glm::mat4));
          SDL_PushGPUFragmentUniformData(cmd_buf, 0, &mouse_pos, sizeof(mouse_pos));

          const SDL_GPUColorTargetInfo col_info = {
            .texture = swapchain_texture,
            .mip_level = 0,
            .layer_or_depth_plane = 0,
            .clear_color = SDL_FColor{ 1.0f, 0.0f, 0.0f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .cycle = false,
          };

          SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info, 1, NULL);
          {
            SDL_BindGPUGraphicsPipeline(render_pass, lighting_pipeline);
            SDL_BindGPUVertexStorageBuffers(render_pass, 0, &quad_data_buffer, 1);

            const SDL_GPUTextureSamplerBinding fragment_samplers[2] = {
              { .texture = gpu_texture_a, .sampler = renderer_info.samplers[0] },
              { .texture = gpu_texture_b, .sampler = renderer_info.samplers[0] },
            };
            SDL_BindGPUFragmentSamplers(render_pass, 0, fragment_samplers, 2);

            // Render one, full screen quad.
            SDL_DrawGPUPrimitives(render_pass, 1 * 6, 1, 0, 0);

            // in the main swapchain texture, call renderdrawdata
            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);
          }
          SDL_EndGPURenderPass(render_pass);
        }
      }

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
  SDL_ReleaseGPUTexture(device, gpu_texture_a);
  SDL_ReleaseGPUTexture(device, gpu_texture_b);
  SDL_ReleaseGPUTexture(device, gpu_texture_c);
  SDL_ReleaseGPUTexture(device, msaa_texture);
  SDL_ReleaseGPUTexture(device, msaa_depth_texture);
  SDL_ReleaseGPUTexture(device, custom_texture_out.texture);
  SDL_ReleaseGPUTexture(device, custom_texture_normal_out.texture);
  // SDL_ReleaseGPUTexture(device, viking_texture.texture);
  SDL_ReleaseGPUTransferBuffer(device, sprite_data_transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, quad_data_transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, bone_data_transfer_buffer);
  SDL_ReleaseGPUBuffer(device, sprite_data_buffer);
  SDL_ReleaseGPUBuffer(device, quad_data_buffer);
  SDL_ReleaseGPUBuffer(device, bone_data_buffer);
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
  // load game_code dll
  const auto src_dll = "GameDLL-hot-unlocked.dll";
  const auto dst_dll = "GameDLL-hot-locked.dll"; // when loaded, system processor locks it

  // Load GameDLL.dll on launch
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

    bool rebuild_dll = false;

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

    // Rebuild the dll
    if (rebuild_dll) {
      game_code.valid = false;
      SDL_Log("Rebuild dll...");

      // rebuild_dll
      const auto build_script = "rebuild_dll.bat";
      const auto full_path = std::format("{}assets/scripts/{}", SDL_GetBasePath(), build_script);
      const auto cmd = std::format("{}", full_path);
      const int result = std::system(cmd.c_str());

      if (result != 0) {
        SDL_Log("Build failed...");
      }

      if (result == 0) {
        SDL_Log("Build success...");
        sdl_unload_game_code(&game_code);
        sdl_load_game_code(game_code, src_dll, dst_dll);
      }
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
