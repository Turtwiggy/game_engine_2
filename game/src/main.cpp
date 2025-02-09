
// #include <vulkan/vulkan.h>
// #include <vk_mem_alloc.h>
// #include <vulkan/vk_enum_string_helper.h>
// #include <backends/imgui_impl_sdl3.h>
// #include <backends/imgui_impl_vulkan.h>

#include "sdl_exception.hpp"
#include "sdl_shader.hpp"
#include "sdl_surface.hpp"
using namespace game2d;

#include <SDL3/SDL.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <span>
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

// profiling cpu: TracyClient.cpp is all you need.
// profiling gpu: RenderDoc

// Goals:
// SDL3; input (keyboard) & audio
// SDL3: filesystem.
// RenderThread
// PhysicsThread
// ~100'000 entities chasing you.

// static constexpr Uint64 NS_PER_FIXED_TICK = 7 * 1e6;  // or ~142 ticks per second
// static constexpr Uint64 NS_PER_FIXED_TICK = 16 * 1e6; // or ~62.5 ticks per second
static bool limit_fps = false;
static int fps_limit = 240;

#define SDL_WINDOW_WIDTH 1280
#define SDL_WINDOW_HEIGHT 720

struct GameData
{
  Uint64 counter = 0;
  float dt = 0.0f;
  std::vector<SDL_Event> evts;
};
struct RenderData
{
  bool use_wireframe_mode = false;
  bool use_small_viewport = false;
  bool use_scissor_viewport = false;

  int cur_sampler_idx = 0;
};
GameData game_data;
RenderData rend_data;

std::atomic<bool> running(true);
std::mutex mtx;
std::condition_variable cv_game_t;
std::condition_variable cv_rend_t;
std::condition_variable cv_main_t;
bool game_thread_ready = false;
bool render_thread_ready = false;
int main_thread_waiting_for = 2;
static SDL_Window* window;
static SDL_GPUDevice* device;

void
WaitForGameThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  cv_game_t.wait(lock, [] { return game_thread_ready; });
  game_thread_ready = false;
};

void
SignalGameThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  game_thread_ready = true;
  cv_game_t.notify_one(); // wake up main()
};

void
WaitForRenderThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  cv_rend_t.wait(lock, [] { return render_thread_ready; });
  render_thread_ready = false;
};

void
SignalRenderThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  render_thread_ready = true;
  cv_rend_t.notify_one(); // wake up main
};

void
WaitForMainThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  cv_main_t.wait(lock, [] { return main_thread_waiting_for > 0; });

  // If you're woken up, main thread no longer waiting for you?
  main_thread_waiting_for--;
};

void
SignalMainThread()
{
  std::lock_guard<std::mutex> lock(mtx);
  main_thread_waiting_for = 2; // wait for 2 threads to finish now
  cv_main_t.notify_all();
};

const auto calc_dt_ns = [](Uint64 now, Uint64& past) -> Uint64 {
  Uint64 dt_ns = now - past;
  dt_ns = std::min(dt_ns, Uint64(250 * 1e6)); // avoid spiral
  past = now;
  return dt_ns;
};

void
GameThread()
{
  SDL_Log("(GameThread) SDL_IsMainThread(): %i", SDL_IsMainThread());

  while (running) {
    static Uint64 game_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, game_past);

    const auto& new_evts = game_data.evts;
    for (const SDL_Event& evt : new_evts) {
      if (evt.type == SDL_EVENT_KEY_DOWN) {
        const auto scancode = evt.key.scancode;
        const auto scancode_name = SDL_GetScancodeName(scancode);
        const auto down = evt.key.down;
        const auto repeat = evt.key.repeat;
        SDL_Log("(GameThread) KeyDown %s %i %i", scancode_name, down, repeat);

        if (scancode == SDL_SCANCODE_KP_PLUS)
          game_data.counter++;
        if (scancode == SDL_SCANCODE_KP_MINUS)
          game_data.counter--;
      }
      if (evt.type == SDL_EVENT_KEY_UP) {
        SDL_Log("(GameThread) KeyUp");
      }
    }
    game_data.evts.clear();

    // SDL_Log("(GameThread) Update()");
    const float dt = dt_ns * 1e-9;
    game_data.dt += dt;
    // SDL_Log("Time: %f", game_data.dt);

    SignalGameThread();
    WaitForMainThread(); // wait until the data is copied
  }
};

// Vertex Formats

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

void
RenderThread()
{
  SDL_Log("(RenderThread) SDL_IsMainThread(): %i", SDL_IsMainThread());

  game2d::InitializeAssetLoader();

  auto vert_shader = game2d::LoadShader(device, "TexturedQuad.vert", 0, 0, 0, 0);
  if (vert_shader == NULL) {
    SDL_Log("Failed to create vert shader");
    exit(SDL_APP_FAILURE); // explode
  };

  auto frag_shader = game2d::LoadShader(device, "TexturedQuad.frag", 1, 0, 0, 0);
  if (frag_shader == NULL) {
    SDL_Log("Failed to create frag shader");
    exit(SDL_APP_FAILURE); // explode
  };

  // Load an image.
  // SDL_Surface* image_data = LoadBMP("ravioli.bmp", 4);
  SDL_Surface* image_data = LoadIMG("a_star.png");
  if (!image_data) {
    throw SDLException("Failed to load image.");
    exit(SDL_APP_FAILURE); // explode
  }

  const char* SamplerNames[] = {
    "PointClamp", "PointWrap", "LinearClamp", "LinearWrap", "AnisotropicClamp", "AnisotropicWrap",
  };

  using VertexFinal = PositionTextureVertex;

  const std::vector<SDL_GPUColorTargetDescription> color_target_desc{
    { .format = SDL_GetGPUSwapchainTextureFormat(device, window) },
  };

  const std::vector<SDL_GPUVertexBufferDescription> vertex_buffer_descriptions{
    {
      .slot = 0,
      .pitch = sizeof(VertexFinal),
      .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
      .instance_step_rate = 0,
    },
  };

  // Setup to match the vertex shader layout
  const std::vector<SDL_GPUVertexAttribute> vertex_attributes{
    {
      // xyz is 3 floats
      .location = 0,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
      .offset = 0,
    },
    {
      // uv is a 2 floats
      .location = 1,
      .buffer_slot = 0,
      .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
      .offset = offsetof(VertexFinal, u),
    },
  };

  // Create the pipelines.
  SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
    .vertex_shader = vert_shader,
    .fragment_shader = frag_shader,
    .vertex_input_state = (SDL_GPUVertexInputState){
      .vertex_buffer_descriptions = vertex_buffer_descriptions.data(),
      .num_vertex_buffers = (Uint32)vertex_buffer_descriptions.size(),
  	  .vertex_attributes = vertex_attributes.data(),
			.num_vertex_attributes = (Uint32)vertex_attributes.size(),
    },
    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    .target_info = {
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

  //
  // Start texture transfer buffer
  //
  const auto texture_transfer_buffer_create_info = (SDL_GPUTransferBufferCreateInfo){
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

  //
  // Upload the transfer data to the vertex and index buffer
  //

  auto* upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
  auto* copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);

  const auto v_buffer_location = SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buffer, .offset = 0 };
  const auto v_buffer_region = SDL_GPUBufferRegion{ .buffer = vertex_buffer, .offset = 0, .size = vertex_data_mem_size };
  SDL_UploadToGPUBuffer(copy_pass, &v_buffer_location, &v_buffer_region, false);

  auto offset = vertex_data_mem_size;
  const auto i_buffer_location = SDL_GPUTransferBufferLocation{ .transfer_buffer = transfer_buffer, .offset = offset };
  const auto i_buffer_region = SDL_GPUBufferRegion{ .buffer = index_buffer, .offset = 0, .size = index_data_mem_size };
  SDL_UploadToGPUBuffer(copy_pass, &i_buffer_location, &i_buffer_region, false);

  const auto ti = SDL_GPUTextureTransferInfo{ .transfer_buffer = texture_transfer_buffer, .offset = 0 /* zero out */ };
  const auto tr = SDL_GPUTextureRegion{ .texture = Texture, .w = (Uint32)image_data->w, .h = (Uint32)image_data->h, .d = 1 };
  SDL_UploadToGPUTexture(copy_pass, &ti, &tr, false);

  SDL_EndGPUCopyPass(copy_pass);
  if (!SDL_SubmitGPUCommandBuffer(upload_cmd_buf))
    throw SDLException("Unable to SDL_SubmitGPUCommandBuffer()");
  SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
  SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);

  const SDL_GPUViewport small_viewport = { 160, 120, 320, 240, 0.1f, 1.0f };
  const SDL_Rect scissor_rect = { 320, 240, 320, 240 };

  SignalRenderThread(); // done init()
  while (running) {
    static Uint64 renderer_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, renderer_past);

    // SDL_Log("(RenderThread) Update()");
    {
      SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);
      if (cmd_buf == NULL) {
        throw SDLException("Could not aquire GPU command buffer");
        exit(SDL_APP_FAILURE); // crash
      }

      // https://wiki.libsdl.org/SDL3/SDL_WaitAndAcquireGPUSwapchainTexture
      SDL_GPUTexture* swapchain_texture;
      SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_texture, NULL, NULL);
      if (swapchain_texture == NULL) {
        SDL_Log("Window is minimized...");
        return;
      }

      if (swapchain_texture) {
        SDL_GPUColorTargetInfo col_info = {
          .texture = swapchain_texture,
          .clear_color = (SDL_FColor){ 0.3f, 0.4f, 0.5f, 1.0f },
          .load_op = SDL_GPU_LOADOP_CLEAR,
          .store_op = SDL_GPU_STOREOP_STORE,
        };
        std::vector color_targets = { col_info };
        auto* render_pass = SDL_BeginGPURenderPass(cmd_buf, color_targets.data(), color_targets.size(), NULL);

        if (rend_data.use_small_viewport)
          SDL_SetGPUViewport(render_pass, &small_viewport);

        if (rend_data.use_scissor_viewport)
          SDL_SetGPUScissor(render_pass, &scissor_rect);

        SDL_BindGPUGraphicsPipeline(render_pass, rend_data.use_wireframe_mode ? line_pipeline : fill_pipeline);

        const std::vector<SDL_GPUBufferBinding> bindings{ { .buffer = vertex_buffer, .offset = 0 } };
        SDL_BindGPUVertexBuffers(render_pass, 0, bindings.data(), bindings.size());

        const SDL_GPUBufferBinding idx_buffer_binding = { .buffer = index_buffer, .offset = 0 };
        SDL_BindGPUIndexBuffer(render_pass, &idx_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

        // Clamp the counter
        int counter = rend_data.cur_sampler_idx;
        counter = counter < 0 ? samplers.size() - 1 : counter;
        counter = counter % samplers.size();

        auto& sampler = samplers[counter];
        const SDL_GPUTextureSamplerBinding tex_sampler_binding = { .texture = Texture, .sampler = sampler };
        SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_sampler_binding, 1);

        SDL_DrawGPUIndexedPrimitives(render_pass, index_data.size(), 1, 0, 0, 0);

        SDL_EndGPURenderPass(render_pass);
      }

      const auto submit = SDL_SubmitGPUCommandBuffer(cmd_buf);
      if (!submit)
        throw SDLException("Could not SDL_SubmitGPUCommandBuffer()");
    }

    SignalRenderThread(); // done
  }

  // Cleanup
  SDL_ReleaseGPUGraphicsPipeline(device, fill_pipeline);
  SDL_ReleaseGPUGraphicsPipeline(device, line_pipeline);
  SDL_ReleaseGPUBuffer(device, vertex_buffer);
};

int
main(int argc, char* argv[])
{
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

  // Start threads, innit
  std::thread game_thread(GameThread);
  std::thread render_thread(RenderThread);
  WaitForRenderThread(); // wait for it to init

  // SDL_AddEventWatch(SDL_EventFilter filter, void *userdata)

  while (running) {
    static Uint64 past = SDL_GetTicksNS();
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, past);
    const Uint64 start_s = SDL_GetPerformanceCounter();

    // SDL_PollEvent: MainThread
    static SDL_Event e;
    static std::vector<SDL_Event> evts;
    evts.clear();
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        running = false;
      evts.push_back(e);
    }

    // SDL_Log("(MainThread) waiting for (GameThread)...");
    // SDL_Log("(MainThread) waiting for (RenderThread)...");
    WaitForGameThread();
    WaitForRenderThread();

    // Once both threads finished their work,
    // RenderThread copies the GameThread data in to it's internal structures.
    // The 2 threads are working on their own, and sharing at the end of the frame.
    //
    // How to avoid copying data between threads?
    //
    // 1. Double buffering. Maintain two copies of the data,
    // one for game thread, one for render thread.
    // The game thread writes to one buffer while the render thread reads from the other.
    // After each frame, the buffers are swapped.
    //
    // 2. Read-Write Locks.
    // A read-write lock allows multiple threads to read data
    // simultaneously but ensures exclusive access for writing.
    // This can be useful if the render thread only needs to read data.
    //
    {
      std::lock_guard<std::mutex> lock(mtx);

      // MainThread to GameThread
      game_data.evts = evts;

      // GameThread to RenderThread
      // rend_data.counter = game_data.counter;
      // rend_data.dt = game_data.dt;
      // rend_data.use_wireframe_mode =
      // SDL_Log("(MainThread) counter: %llu", game_data.counter);

      rend_data.use_wireframe_mode = false;
      rend_data.use_scissor_viewport = false;
      rend_data.use_small_viewport = false;
      // if (game_data.counter == 1)
      //   rend_data.use_wireframe_mode = true;
      // if (game_data.counter == 2)
      //   rend_data.use_scissor_viewport = true;
      // if (game_data.counter == 3)
      //   rend_data.use_small_viewport = true;

      rend_data.cur_sampler_idx = game_data.counter;
    }

    // SDL_Log("(MainThread) Done");
    SignalMainThread();
  }

  game_thread.join();
  render_thread.join();

  SDL_ReleaseWindowFromGPUDevice(device, window);
  SDL_DestroyWindow(window);
  SDL_DestroyGPUDevice(device);

  return 0;
};

// Note: can use std::async for parallelisable tasks...
// e.g.
// auto player_tasks = std::async(std::launch::async,
//   [](){
//       input->QueryPlayerInput();
//       player->UpdatePlayerCharacter();
//   });
// player_tasks.wait();

// Physics...
// accu += dt_ns;
// while (accu >= NS_PER_FIXED_TICK) {
//   accu -= NS_PER_FIXED_TICK;
//   // fixedupdate
// }

// particles is standalone AND we can update each particle on its own, so we can combine async with parallel for
// auto particles_task = std::async(std::launch::async,
//     [](){
//         std::for_each(std::execution::par,
//             ParticleSystems.begin(),
//             ParticleSystems.end(),
//             [](Particle* Part){
//                  Part->UpdateParticles();

//                 if(Part->IsDead)
//                 {
//                     deletion_queue.push(Part);
//                 }
//             });
//     });

// };