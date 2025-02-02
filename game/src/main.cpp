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

// #include <vk_mem_alloc.h>
// #include <vulkan/vk_enum_string_helper.h>
// #include <vulkan/vulkan.h>
// #include <backends/imgui_impl_sdl3.h>
// #include <backends/imgui_impl_vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

// static constexpr Uint64 NS_PER_FIXED_TICK = 7 * 1e6;  // or ~142 ticks per second
// static constexpr Uint64 NS_PER_FIXED_TICK = 16 * 1e6; // or ~62.5 ticks per second
static bool limit_fps = true;
static int fps_limit = 240;

#define SDL_WINDOW_WIDTH 1280
#define SDL_WINDOW_HEIGHT 720

typedef struct
{
  SDL_Window* window;
  SDL_Renderer* renderer;
} AppState;

struct GameData
{
  Uint64 counter = 0;
  float dt = 0.0f;
};
struct RenderData
{
  Uint64 counter = 0;
  float dt = 0.0f;
};
GameData game_data;
RenderData rend_data;

std::atomic<bool> running(true);
std::mutex mtx;
std::condition_variable cv;
bool render_thread_ready = false;
bool game_thread_ready = false;

void
WaitForRenderThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [] { return render_thread_ready; });
  render_thread_ready = false;
};

void
SignalRenderThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  render_thread_ready = true; // Signal that the render thread has finished
  cv.notify_one();            // Notify the game thread
};

void
WaitForGameThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [] { return game_thread_ready; });
  game_thread_ready = false;
};

void
SignalGameThread()
{
  std::unique_lock<std::mutex> lock(mtx);
  game_thread_ready = true;
  cv.notify_one();
};

void
UpdateGameLogic(Uint64 dt_ns) {
  // SDL_Delay(16); // work

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

  // Update Game Systems...
  // Poll Input...
  // Update()
};

void
UpdateRenderLogic(SDL_Renderer* renderer, const RenderData& data, Uint64 dt_ns)
{
  // SDL_Delay(16); // work

  // update (NS_DT)
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
  SDL_RenderClear(renderer);

  char str[32];

  // Display: fps
  const float fps = 1e9 / dt_ns;
  SDL_snprintf(str, sizeof(str), "%0.2f FPS", fps);
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderDebugText(renderer, 0, 0, str);

  // Display: fps limit
  SDL_snprintf(str, sizeof(str), "FPS Limit: %i", fps_limit);
  SDL_RenderDebugText(renderer, 0, 16, str);

  // Display: timer
  SDL_snprintf(str, sizeof(str), "Timer: %0.2f", data.dt);
  SDL_RenderDebugText(renderer, 0, 32, str);

  // Done
  SDL_RenderPresent(renderer);
};

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
void
CopyGameThreadDataToRenderer()
{
  std::unique_lock<std::mutex> lock(mtx);

  // auto view = gameRegistry.view<Position>();
  // if (!view.empty()) {
  //   renderData.position = pos;

  rend_data.counter = game_data.counter;
  rend_data.dt = game_data.dt;
};

static AppState as;

SDL_AppResult
init()
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

  if (!SDL_SetAppMetadata("Example Snake game", "1.0", "com.blueberrygames.game"))
    return SDL_APP_FAILURE;

  SDL_Log("Init SDL3()");

  if (!SDL_Init(SDL_INIT_VIDEO))
    return SDL_APP_FAILURE;

  if (!SDL_CreateWindowAndRenderer("game", SDL_WINDOW_WIDTH, SDL_WINDOW_HEIGHT, 0, &as.window, &as.renderer))
    return SDL_APP_FAILURE;

  return SDL_APP_CONTINUE;
};

const auto calc_dt_ns = [](Uint64 now, Uint64& past) -> Uint64 {
  Uint64 dt_ns = now - past;
  dt_ns = std::min(dt_ns, Uint64(250 * 1e6)); // avoid spiral
  past = now;
  return dt_ns;
};

void
GameLoop()
{
  SDL_Log("Hello from GameLoopThread()");

  // Called from the GameThread, as
  // SDL_PollEvent may call SDL_PumpEvents can
  // only be called from the thread that sets the videomode.
  init();

  while (running) {
    static Uint64 game_past = 0;
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, game_past);

    static SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT)
        running = false;
    };

    UpdateGameLogic(dt_ns);
    const float dt = dt_ns * 1e-9;
    game_data.counter++;
    game_data.dt += dt;

    SignalGameThread();
    WaitForRenderThread();
  }
};

int
main(int argc, char* argv[])
{
  std::thread game_thread(GameLoop);

  while (running) {
    static Uint64 past = SDL_GetTicksNS();
    const Uint64 now = SDL_GetTicksNS();
    const Uint64 dt_ns = calc_dt_ns(now, past);

    UpdateRenderLogic(as.renderer, rend_data, dt_ns);

    // Note: limiting renderthread fps would also
    // cause the gamethread to slow down execution which seems weird,
    // because gamethread waits for renderthread
    // if (limit_fps) {
    //   const Uint64 end = SDL_GetTicksNS();
    //   const Uint64 elapsed_ns = (end - now);
    //   const Uint64 target_ns = 1e9 / fps_limit;
    //   const Uint64 delay_ns = SDL_floor(target_ns - elapsed_ns);
    //   if (delay_ns > 0)
    //     SDL_DelayNS(delay_ns);
    // }

    WaitForGameThread(); // todo: handle running = false

    // Once both threads finished their work,
    // RenderThread copies the GameThread data in to it's internal structures.
    // The 2 threads are working on their own, and sharing at the end of the frame.
    CopyGameThreadDataToRenderer();

    SignalRenderThread();
  }

  game_thread.join();

  return 0;
};