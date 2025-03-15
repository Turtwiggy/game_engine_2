
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "box2d_parallel.hpp"
#include "common.hpp"
#include "sdl_exception.hpp"
#include "sdl_hot_reload_dll.hpp"
#include "sdl_shader.hpp"
#include "sdl_surface.hpp"
#include "threadsafe_queue.hpp"
#include <mutex>
using namespace game2d;

#if !defined(TRACY_ENABLE)
#define TRACY_ENABLE
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <box2d/box2d.h>
#include <box2d/id.h>
#include <entt/entt.hpp>
#include <tracy/Tracy.hpp>

// #include <backends/imgui_impl_vulkan.h>
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_sdlgpu3.h"
#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <vector>

// clang-format off

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

// What it takes to draw a triangle.
// 1. Physical device selection
// 2. Logical device and queue families
// 3. Window surface and swap chain
// 4. Image views and framebuffers
// 5. Render passes
// 6. Graphics pipeline
// 7. Command pools and command buffers
// 8. Main Loop

// VkInstance: The Vulkan context, used to access drivers.
// VkPhysicalDevice: A GPU. Used to query physical GPU details, like features, capabilities, memory size, etc.
// VkDevice: The “logical” GPU context that you actually execute things on.
// VkBuffer: A chunk of GPU visible memory.
// VkImage: A texture you can write to and read from.
// VkPipeline: Holds the state of the gpu needed to draw. For example: shaders, rasterization options, depth settings.
// VkRenderPass: Holds information about the images you are rendering into. All drawing commands have to be done inside a renderpass. Only used in legacy vkguide
// VkFrameBuffer: Holds the target images for a renderpass. Only used in legacy vkguide
// VkCommandBuffer: Encodes GPU commands. All execution that is performed on the GPU itself (not in the driver) has to be encoded in a VkCommandBuffer.
// VkQueue: Execution “port” for commands. GPUs will have a set of queues with different properties. Some allow only graphics commands, others only allow memory commands, etc. Command buffers are executed by submitting them into a queue, which will copy the rendering commands onto the GPU for execution.
// VkDescriptorSet: Holds the binding information that connects shader inputs to data such as VkBuffer resources and VkImage textures. Think of it as a set of gpu-side pointers that you bind once.
// VkSwapchainKHR: Holds the images for the screen. It allows you to render things into a visible window. The KHR suffix shows that it comes from an extension, which in this case is VK_KHR_swapchain
// VkSemaphore: Synchronizes GPU to GPU execution of commands. Used for syncing multiple command buffer submissions one after other.
// VkFence: Synchronizes GPU to CPU execution of commands. Used to know if a command buffer has finished being executed on the GPU.

// clang-format on

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
constexpr int SDL_WINDOW_WIDTH = 1280;
constexpr int SDL_WINDOW_HEIGHT = 720;

struct RenderData
{
  std::mutex mtx; // mutex to protect access to data

  // data
  std::vector<TransformComponent> transforms;
};

//
// idea:
// read buffer 0
// write to buffer 1
// swap the buffers at the end of the game update,
// ensuring the renderer always sees a stable snapshot.
//
// clang-format off

GameUIData game_ui_data;
RenderData rend_data[2];
std::atomic<vec2> mouse_pos;
std::atomic<int> current_read_buffer = 0;
RenderData& GetWriteBuffer(){return rend_data[1 - current_read_buffer.load(std::memory_order_acquire)];};
RenderData& GetReadBuffer(){return rend_data[current_read_buffer.load(std::memory_order_acquire)];};
void SwapBuffers() {current_read_buffer.store(1 - current_read_buffer.load(), std::memory_order_release);};

sdl_game_code game_code;
std::atomic<bool> game_code_valid{false};
std::mutex game_code_mtx;

// clang-format on

EventQueue<SDL_Event> event_queue;
std::atomic<bool> running(true);
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

  if (!game_code_valid.load()) {
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

  b2WorldDef world_def = b2DefaultWorldDef();
  // world_def.workerCount = worker_count;
  // world_def.enqueueTask = EnqueueTask;
  // world_def.finishTask = FinishTask;
  // world_def.userTaskContext = s.get();
  world_def.gravity = { 0.0f, 0.0f };
  world_def.enableSleep = true;

  //  game init after physics init
  auto world_id = b2CreateWorld(&world_def);

  entt::registry r;
  GameData game_data{ .r = r, .world_id = world_id };

  {
    std::scoped_lock<std::mutex> lock(game_code_mtx);
    game_code.game_init(&game_data);
  }

  SDL_Log("(GameThread) -- done init");
  tracy::SetThreadName("GameThread");

  while (running) {
    static Uint64 game_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, game_past);
    const float dt = 1e-9 * (float)dt_ns; // inaccurate, but for game
    ZoneScopedN("GameThread");

    // pop all the events at once from a thread-safe buffer.
    {
      std::scoped_lock lock(game_code_mtx);
      game_data.events = event_queue.dequeue_all();
      game_data.dt = dt;
      game_data.mouse_pos = mouse_pos.load();
    }

    // update ui data
    {
      std::scoped_lock lock(game_ui_data.mtx);
      game_ui_data.game_dt = dt;
    }

    // run physics at fixed timesteps
    static Uint64 accu = 0;
    constexpr int physics_substep_count = 4;
    constexpr float physics_dt = 1.0f / 60.0f;
    constexpr Uint64 NS_PER_FIXED_TICK = 1e9 / 60.0f;
    accu += dt_ns;
    while (accu >= NS_PER_FIXED_TICK) {
      accu -= NS_PER_FIXED_TICK;

      // FixedUpdate()
      {
        std::scoped_lock<std::mutex> lock(game_code_mtx);
        if (game_code_valid.load())
          game_code.game_fixed_update(&game_data);
      }

      b2World_Step(game_data.world_id, physics_dt, physics_substep_count);
      // s->m_taskCount = 0;
    }

    // GameUpdate()

    for (const auto& evt : game_data.events) {
      if (evt.type == SDL_EVENT_KEY_DOWN) {
        const auto scancode = evt.key.scancode;
        if (scancode == SDL_SCANCODE_9) {
          {
            std::scoped_lock<std::mutex> lock(game_code_mtx);
            if (game_code_valid.load())
              game_code.game_refresh(&game_data);
          }
        }
      }
    }

    {
      std::scoped_lock<std::mutex> lock(game_code_mtx);
      if (game_code_valid.load())
        game_code.game_update(&game_data);
    }

    // Ding ding! frame done. UpdateRenderData
    const int write_buffer_index = 1 - current_read_buffer.load(std::memory_order_acquire);
    RenderData& wb = rend_data[write_buffer_index];
    {
      std::scoped_lock<std::mutex> lock0(wb.mtx);

      auto view = game_data.r.view<TransformComponent>();
      wb.transforms.clear(); // should do something better than .clear()
      const auto get_comp = [](const auto& tuple) -> TransformComponent { return std::get<1>(tuple); };
      std::ranges::copy(view.each() | std::views::transform(get_comp), std::back_inserter(wb.transforms));
    }

    SwapBuffers();
    FrameMark; // frame done
  }

  b2DestroyWorld(game_data.world_id);
};

// Vertex Formats

/*

typedef struct PositionVertex
{
  float x, y, z;
} PositionVertex;

typedef struct PositionColorVertex
{
  float x, y, z;
  Uint8 r, g, b, a;
} PositionColorVertex;

typedef struct PositionTextureVertex
{
  float x, y, z;
  float u, v;
} PositionTextureVertex;

typedef struct Index
{
  Uint16 i;
} Index;

// Rect
const std::vector<PositionTextureVertex> vertex_data{
  // clang-format off
  // xyz, uv
  // { -0.5, -0.5, 0,      0.0f, 1.0f },   //
  // { 0.5, -0.5, 0,       1.0f, 1.0f },   //
  // { 0.5, 0.5, 0,        1.0f, 1.0f },   //
  // { -0.5, 0.5, 0,       0.0f, 1.0f },   //
  {-0.5, 0.5, 0, 0, 0},
  {0.5, 0.5, 0, 1, 0},
  {0.5, -0.5, 0, 1, 1},
  {-0.5, -0.5, 0, 0, 1} ,
  // clang-format on
};

const std::vector<Index> index_data{
  { 0 }, { 1 }, { 2 }, //
  { 0 }, { 2 }, { 3 },
};

*/

typedef struct SpriteInstance
{
  float x, y, z;
  float rotation;
  float w, h, padding_a, padding_b;
  float tex_u, tex_v, tex_w, tex_h;
  float r, g, b, a;
} SpriteInstance;

typedef struct Matrix4x4
{
  float m11, m12, m13, m14;
  float m21, m22, m23, m24;
  float m31, m32, m33, m34;
  float m41, m42, m43, m44;
} Matrix4x4;

Matrix4x4
Matrix4x4_CreateOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
{
  // clang-format off
  return Matrix4x4{
		2.0f / (right - left), 0, 0, 0,
		0, 2.0f / (top - bottom), 0, 0,
		0, 0, 1.0f / (zNearPlane - zFarPlane), 0,
		(left + right) / (left - right), (top + bottom) / (bottom - top), zNearPlane / (zNearPlane - zFarPlane), 1
	};
  // clang-format on
}

void
RenderThread()
{
  const auto info_str = std::format("(RenderThread) SDL_IsMainThread(): {}", SDL_IsMainThread());
  SDL_Log("%s", info_str.c_str());

  game2d::InitializeAssetLoader();

  const uint32_t SPRITE_COUNT = 8192;
  const Matrix4x4 cameraMatrix = Matrix4x4_CreateOrthographicOffCenter(0, 1280, 720, 0, 0, -1);

  SDL_GPUPresentMode present_mode = SDL_GPU_PRESENTMODE_VSYNC;
  if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_IMMEDIATE))
    present_mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
  else if (SDL_WindowSupportsGPUPresentMode(device, window, SDL_GPU_PRESENTMODE_MAILBOX))
    present_mode = SDL_GPU_PRESENTMODE_MAILBOX;
  SDL_SetGPUSwapchainParameters(device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, present_mode);

  auto vert_shader = game2d::LoadShader(device, "PullSpriteBatch.vert", 0, 1, 1, 0);
  if (vert_shader == NULL) {
    SDL_Log("Failed to create vert shader");
    exit(SDL_APP_FAILURE); // explode
  };

  auto frag_shader = game2d::LoadShader(device, "TexturedQuadColor.frag", 1, 0, 0, 0);
  if (frag_shader == NULL) {
    SDL_Log("Failed to create frag shader");
    exit(SDL_APP_FAILURE); // explode
  };

  const char* SamplerNames[] = {
    "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  };

  // using VertexFinal = PositionTextureVertex;

  const std::vector<SDL_GPUColorTargetDescription> color_target_desc{
      {.format = SDL_GetGPUSwapchainTextureFormat(device, window),
       .blend_state =
           {
               .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
               .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
               .color_blend_op = SDL_GPU_BLENDOP_ADD,
               .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
               .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
               .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
               .enable_blend = true,
           }},
  };

  // const std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_descriptions{
  //   {
  //     .slot = 0,
  //     .pitch = sizeof(VertexFinal),
  //     .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
  //     .instance_step_rate = 0,
  //   },
  // };

  // Setup to match the vertex shader layout
  // const std::vector<SDL_GPUVertexAttribute> vertex_attributes{
  //   {
  //     // xyz is 3 floats
  //     .location = 0,
  //     .buffer_slot = 0,
  //     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
  //     .offset = 0,
  //   },
  //   {
  //     // uv is a 2 floats
  //     .location = 1,
  //     .buffer_slot = 0,
  //     .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
  //     .offset = offsetof(VertexFinal, u),
  //   },
  // };

  // Create the pipelines.
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
      .vertex_shader = vert_shader,
      .fragment_shader = frag_shader,
      // .vertex_input_state = (SDL_GPUVertexInputState){
      //   .vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
      //   .num_vertex_buffers = (Uint32)vertex_buffer_descriptions.size(),
      //   .vertex_attributes = vertex_attributes.data(),
      // 	.num_vertex_attributes = (Uint32)vertex_attributes.size(),
      // },
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
      .target_info =
          {
              .color_target_descriptions = color_target_desc.data(),
              .num_color_targets = (Uint32)color_target_desc.size(),
          },
  };

  // can release shaders after creating pipelines
  SDL_Log("Releasing shaders... be free!");
  SDL_ReleaseGPUShader(device, vert_shader);
  SDL_ReleaseGPUShader(device, frag_shader);

  pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  auto* fill_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
  if (fill_pipeline == nullptr) {
    throw SDLException("Failed to create Fill GraphicsPipeline()");
    exit(SDL_APP_FAILURE); // crash
  }

  pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
  auto* line_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
  if (line_pipeline == nullptr) {
    throw SDLException("Failed to create Line GraphicsPipeline()");
    exit(SDL_APP_FAILURE); // crash
  }

  const auto s0 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  const auto s1 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };
  const auto s2 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  };
  const auto s3 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
  };
  const auto s4 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .max_anisotropy = 4,
    .enable_anisotropy = true,
  };
  const auto s5 = SDL_GPUSamplerCreateInfo{
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .max_anisotropy = 4,
    .enable_anisotropy = true,
  };

  //  "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  const auto samplers = std::vector{
    SDL_CreateGPUSampler(device, &s0), SDL_CreateGPUSampler(device, &s1), SDL_CreateGPUSampler(device, &s2),
    SDL_CreateGPUSampler(device, &s3), SDL_CreateGPUSampler(device, &s4), SDL_CreateGPUSampler(device, &s5),
  };

  /*
  const Uint32 vertex_data_mem_size = sizeof(VertexFinal) * vertex_data.size();
  const Uint32 index_data_mem_size = sizeof(Index) * index_data.size();

  // Create the vertex buffer
  const auto vertex_buffer_info = (SDL_GPUBufferCreateInfo){
    .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
    .size = vertex_data_mem_size,
  };
  auto vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
  if (!vertex_buffer)
    throw SDLException("Failed to create GpuBuffer");
  SDL_SetGPUBufferName(device, vertex_buffer, "VertexBuffer");

  // Create an index buffer
  const auto index_buffer_info = (SDL_GPUBufferCreateInfo){
    .usage = SDL_GPU_BUFFERUSAGE_INDEX,
    .size = index_data_mem_size,
  };
  auto index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
  if (!index_buffer)
    throw SDLException("Failed to create GpuBuffer");
  SDL_SetGPUBufferName(device, index_buffer, "IndexBuffer");

//
  // To get data in to the vertex buffer, we have to use a transfer buffer.
  //
  const auto transfer_buffer_info = (SDL_GPUTransferBufferCreateInfo){
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = vertex_data_mem_size + index_data_mem_size,
  };

  // Map the buffer in to cpu memory
  auto* transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);
  if (!transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  // Copy the data in

  auto* ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

  std::span vertex_buffer_data = { reinterpret_cast<VertexFinal*>(ptr), vertex_data.size() };
  std::ranges::copy(vertex_data, vertex_buffer_data.begin());

  std::span index_buffer_data = { reinterpret_cast<Index*>(ptr + vertex_data_mem_size), index_data.size() };
  std::ranges::copy(index_data, index_buffer_data.begin());

  SDL_UnmapGPUTransferBuffer(device, transfer_buffer); // note: need to unmap before aquire gpu command

  */

  // Load an image.
  // SDL_Surface* image_data = LoadBMP("ravioli.bmp", 4);
  SDL_Surface* image_data = LoadIMG("a_star.png");
  if (!image_data) {
    throw SDLException("Failed to load image.");
    exit(SDL_APP_FAILURE); // explode
  }

  // Create a texture
  SDL_GPUTextureCreateInfo texture_create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = static_cast<Uint32>(image_data->w),
    .height = static_cast<Uint32>(image_data->h),
    .layer_count_or_depth = 1,
    .num_levels = 1,
  };
  auto* Texture = SDL_CreateGPUTexture(device, &texture_create_info);
  if (!Texture) {
    throw SDLException("Failed to CreateGPUTexture()");
    exit(SDL_APP_FAILURE); // crash
  }
  SDL_SetGPUTextureName(device, Texture, "RavioliTexture");

  // Start texture transfer buffer
  const auto texture_transfer_buffer_create_info = SDL_GPUTransferBufferCreateInfo{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = (Uint32)(image_data->w * image_data->h * 4),
  };

  // Map the buffer in to cpu memory
  auto* texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_create_info);
  if (!texture_transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  // Copy data in to transfer buffer
  auto* texture_ptr = (Uint8*)SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
  SDL_memcpy(texture_ptr, image_data->pixels, image_data->w * image_data->h * 4);
  SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);

  const auto sprite_data_transfer_buffer_info = SDL_GPUTransferBufferCreateInfo{
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = SPRITE_COUNT * sizeof(SpriteInstance),
  };
  auto* sprite_data_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &sprite_data_transfer_buffer_info);
  if (!sprite_data_transfer_buffer)
    throw SDLException("Unable to SDL_CreateGPUTransferBuffer()");

  const auto sprite_data_buffer_info = SDL_GPUBufferCreateInfo{
    .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
    .size = SPRITE_COUNT * sizeof(SpriteInstance),
  };
  auto* sprite_data_buffer = SDL_CreateGPUBuffer(device, &sprite_data_buffer_info);
  if (!sprite_data_buffer)
    throw SDLException("Unable to SDL_CreateGPUBuffer()");

  // Copy data
  auto* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
  auto* copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);

  // const auto v_buffer_location = SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buffer, .offset = 0 };
  // const auto v_buffer_region = SDL_GPUBufferRegion{ .buffer = vertex_buffer, .offset = 0, .size = vertex_data_mem_size
  // }; SDL_UploadToGPUBuffer(copy_pass, &v_buffer_location, &v_buffer_region, false);

  // auto offset = vertex_data_mem_size;
  // const auto i_buffer_location = SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buffer, .offset = offset };
  // const auto i_buffer_region = SDL_GPUBufferRegion{ .buffer = index_buffer, .offset = 0, .size = index_data_mem_size };
  // SDL_UploadToGPUBuffer(copy_pass, &i_buffer_location, &i_buffer_region, false);

  const auto ti = SDL_GPUTextureTransferInfo{ .transfer_buffer = texture_transfer_buffer, .offset = 0 /* zero out */ };
  const auto tr = SDL_GPUTextureRegion{ .texture = Texture, .w = (Uint32)image_data->w, .h = (Uint32)image_data->h, .d = 1 };
  SDL_UploadToGPUTexture(copy_pass, &ti, &tr, false);

  SDL_EndGPUCopyPass(copy_pass);
  if (!SDL_SubmitGPUCommandBuffer(upload_cmd_buf))
    throw SDLException("Unable to SDL_SubmitGPUCommandBuffer()");
  // SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);

  const SDL_GPUViewport small_viewport = { 160, 120, 320, 240, 0.1f, 1.0f };
  const SDL_Rect scissor_rect = { 320, 240, 320, 240 };

  SDL_Log("(RenderThread) -- done init");
  // SignalRenderThread(); // done init()
  // WaitForMainThread();

  tracy::SetThreadName("RenderThread");
  while (running) {
    ZoneScopedN("RenderThread");

    static Uint64 renderer_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, renderer_past);

    // note: this doubles the memory,
    // because its copying the entire RenderData
    auto& read_buffer = GetReadBuffer();
    RenderData rd;
    {
      std::scoped_lock<std::mutex> lock(read_buffer.mtx);

      // take a copy
      rd.transforms = read_buffer.transforms;
    }
    const auto& transforms = rd.transforms;

    // Start the Dear ImGui frame
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(NULL);

    {
      std::scoped_lock<std::mutex> lock0(game_code_mtx);
      std::scoped_lock<std::mutex> lock1(game_ui_data.mtx);
      if (game_code_valid.load())
        game_code.game_update_ui(&game_ui_data);
    }

    // if (transforms.size() > 0) {
    //   const auto msg = std::format("(RT) transforms: {}", transforms.size());
    //   SDL_Log("%s", msg.c_str());
    // }

    // SDL_Log("(RenderThread) Update()");
    {
      ImGui::Render();
      ImDrawData* draw_data = ImGui::GetDrawData();
      const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

      SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);
      if (cmd_buf == nullptr) {
        throw SDLException("Could not aquire GPU command buffer");
        exit(SDL_APP_FAILURE); // crash
      }

      // https://wiki.libsdl.org/SDL3/SDL_WaitAndAcquireGPUSwapchainTexture
      SDL_GPUTexture* swapchain_texture;
      SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_texture, nullptr, nullptr);

      if (swapchain_texture != nullptr && !is_minimized) {
        // This is mandatory: call Imgui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
        Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, cmd_buf);

        // Build sprite instance transfer
        SpriteInstance* data_ptr = (SpriteInstance*)SDL_MapGPUTransferBuffer(device, sprite_data_transfer_buffer, true);
        for (Uint32 i = 0; i < SPRITE_COUNT; i += 1) {
          if (i < transforms.size()) {
            data_ptr[i].x = transforms[i].pos.x;
            data_ptr[i].y = transforms[i].pos.y;
            data_ptr[i].z = 0.0f;
            data_ptr[i].rotation = transforms[i].rotation_radians;
            data_ptr[i].w = transforms[i].size.x;
            data_ptr[i].h = transforms[i].size.y;
          } else {

            // this should not occur right now,
            // as we have two transforms.
            // why is this occuring?
            if (i == 0) {
              int k = 1;
            }

            data_ptr[i].x = 0.0f;
            data_ptr[i].y = 0.0f;
            data_ptr[i].z = 0.0f;
            data_ptr[i].rotation = 0.0f;
            data_ptr[i].w = 0.0f;
            data_ptr[i].h = 0.0f;
          }

          data_ptr[i].padding_a = 0.0f;
          data_ptr[i].padding_b = 0.0f;
          data_ptr[i].tex_u = 0.0f;
          data_ptr[i].tex_v = 0.0f;
          data_ptr[i].tex_w = 1.0f;
          data_ptr[i].tex_h = 1.0f;
          data_ptr[i].r = 1.0f;
          data_ptr[i].g = 1.0f;
          data_ptr[i].b = 1.0f;
          data_ptr[i].a = 1.0f;
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

        // Render sprites.
        SDL_GPUColorTargetInfo col_info = {
          .texture = swapchain_texture,
          .mip_level = 0,
          .layer_or_depth_plane = 0,
          .clear_color = SDL_FColor{ 0.3f, 0.4f, 0.5f, 1.0f },
          .load_op = SDL_GPU_LOADOP_CLEAR,
          .store_op = SDL_GPU_STOREOP_STORE,
          .cycle = false,
        };
        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmd_buf, &col_info, 1, NULL);

        // if (read_buffer.use_small_viewport)
        //   SDL_SetGPUViewport(render_pass, &small_viewport);
        // if (read_buffer.use_scissor_viewport)
        //   SDL_SetGPUScissor(render_pass, &scissor_rect);

        SDL_BindGPUGraphicsPipeline(render_pass, fill_pipeline);
        SDL_BindGPUVertexStorageBuffers(render_pass, 0, &sprite_data_buffer, 1);

        // const std::vector<SDL_GPUBufferBinding> bindings{ { .buffer = vertex_buffer, .offset = 0 } };
        // SDL_BindGPUVertexBuffers(render_pass, 0, bindings.data(), bindings.size());

        // const SDL_GPUBufferBinding idx_buffer_binding = { .buffer = index_buffer, .offset = 0 };
        // SDL_BindGPUIndexBuffer(render_pass, &idx_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        auto& sampler = samplers[0];
        const SDL_GPUTextureSamplerBinding tex_sampler_binding = { .texture = Texture, .sampler = sampler };
        SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_sampler_binding, 1);

        SDL_PushGPUVertexUniformData(cmd_buf, 0, &cameraMatrix, sizeof(Matrix4x4));

        SDL_DrawGPUPrimitives(render_pass, SPRITE_COUNT * 6, 1, 0, 0);
        // SDL_DrawGPUIndexedPrimitives(render_pass, index_data.size(), 1, 0, 0, 0);

        // Render ImGui
        ImGui_ImplSDLGPU3_RenderDrawData(draw_data, cmd_buf, render_pass);

        SDL_EndGPURenderPass(render_pass);
      }

      const auto submit = SDL_SubmitGPUCommandBuffer(cmd_buf);
      if (!submit)
        throw SDLException("Could not SDL_SubmitGPUCommandBuffer()");
    }

    FrameMark; // frame done
  }

  // Cleanup
  SDL_ReleaseGPUGraphicsPipeline(device, fill_pipeline);
  SDL_ReleaseGPUGraphicsPipeline(device, line_pipeline);
  SDL_ReleaseGPUTexture(device, Texture);
  SDL_ReleaseGPUTransferBuffer(device, sprite_data_transfer_buffer);
  SDL_ReleaseGPUBuffer(device, sprite_data_buffer);
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

#if defined(SDL_PLATFORM_WIN32)
  SDL_Log("Hello, Windows!");
#elif defined(SDL_PLATFORM_MACOS)
  SDL_Log("Hello, Mac!");
#elif defined(SDL_PLATFORM_LINUX)
  SDL_Log("Hello, Linux!");
#elif defined(SDL_PLATFORM_IOS)
  SDL_Log("Hello, iOS!");
#elif defined(SDL_PLATFORM_ANDROID);
  SDL_Log("Hello, Android!");
#else
  SDL_Log("Hello, Unknown platform!");
#endif

  SDL_Log("Hello, MainThread!");
  SDL_Log("You have %i logical cpu cores", SDL_GetNumLogicalCPUCores());
  SDL_Log("(main()) SDL_IsMainThread(): %i", SDL_IsMainThread());

  if (!SDL_SetAppMetadata("Example Snake game", "1.0", "com.blueberrygames.game"))
    throw SDLException("Couldn't SDL_SetAppMetadata()");

  if (!SDL_Init(SDL_INIT_VIDEO))
    throw SDLException("Failed to SDL_Init(SDL_INIT_VIDEO)");

  auto flags = SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL | SDL_GPU_SHADERFORMAT_DXIL;
#if defined(_DEBUG)
  device = { SDL_CreateGPUDevice(flags, true, nullptr) };
#else
  device = { SDL_CreateGPUDevice(flags, false, nullptr) };
#endif
  if (device == NULL)
    throw SDLException("Couldn't create GPU device");

  // Call this only on the MainThread
  SDL_Log("(RenderThread) -- CreateWindow");

  // SDL_CreateWindow: main thread
  window = SDL_CreateWindow("Game", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0);
  if (window == NULL)
    throw SDLException("Couldn't SDL_CreateWindow()");

  SDL_ShowWindow(window);
  SDL_SetWindowResizable(window, true);

  if (!SDL_ClaimWindowForGPUDevice(device, window))
    throw SDLException("Couldn't claim window for GPU device");

  auto device_driver = SDL_GetGPUDeviceDriver(device);
  SDL_Log("Using GPU device driver: %s", device_driver);

  //
  // Setup ImGUI
  //

  // clang-format off

  IMGUI_CHECKVERSION();
  game_ui_data.ctx = ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLGPU(window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = device;
  init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(device, window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
  ImGui_ImplSDLGPU3_Init(&init_info);

  // clang-format on

  // #elif __linux__
  //   "libGameDLL.so";
  // #elif __APPLE__
  //   "libGameDLL.dylib";
  // load game_code dll
  const auto src_dll = "GameDLL.dll";
  const auto dst_dll = "GameDLL-locked.dll"; // when loaded, system processor locks it
  {
    std::scoped_lock<std::mutex> lock(game_code_mtx);
    game_code = sdl_load_game_code(src_dll, dst_dll);
    game_code_valid.store(true);
  }

  // Start threads, innit
  std::thread game_thread(GameThread);
  std::thread render_thread(RenderThread);

  while (running) {
    static Uint64 past = SDL_GetTicksNS();
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, past);
    const Uint64 start_s = SDL_GetPerformanceCounter();
    ZoneScopedN("MainThread");

    bool rebuild_dll = false;

    // SDL_PollEvent: MainThread
    static SDL_Event e;
    static std::vector<SDL_Event> evts;
    evts.clear();
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL3_ProcessEvent(&e);

      if (e.type == SDL_EVENT_QUIT)
        running = false;
      if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && e.window.windowID == SDL_GetWindowID(window))
        running = false;

      if (e.type == SDL_EVENT_KEY_DOWN) {
        const auto scancode = e.key.scancode;
        if (scancode == SDL_SCANCODE_9)
          rebuild_dll = true;
      }

      evts.push_back(e);
    }

    // push all the events at once in to a thread-safe vector.
    event_queue.enqueue(evts);

    // Call SDL_GetMouseState on main thread.
    float x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    mouse_pos.store({ x, y });

    // Rebuild the dll
    if (rebuild_dll) {
      std::scoped_lock<std::mutex> lock(game_code_mtx);

      SDL_Log("Rebuild dll...");

      // rebuild_dll
      const auto build_script = "rebuild_dll.bat";
      const auto full_path = std::format("{}assets/scripts/{}", SDL_GetBasePath(), build_script);
      const auto cmd = std::format("{}", full_path);
      const int result = std::system(cmd.c_str());

      if (result != 0)
        SDL_Log("Build failed...");

      if (result == 0) {
        SDL_Log("Build success...");
        game_code_valid.store(false);
        sdl_unload_game_code(&game_code);
        game_code = sdl_load_game_code(src_dll, dst_dll);
        game_code_valid.store(true);
      }
    }

    FrameMark; // frame done
  }

  game_thread.join();
  render_thread.join();

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);

  return 0;
};
