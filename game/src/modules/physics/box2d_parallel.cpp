#include "pch.hpp"

#include "box2d_parallel.hpp"

#include "TaskScheduler.h"

namespace game2d {

struct SampleContext
{
  int workerCount = 1;
};

class Sample
{
public:
  static constexpr int m_maxTasks = 64;
  // static constexpr int m_maxThreads = 64;

  SampleContext* m_context;

  enki::TaskScheduler* m_scheduler;
  class SampleTask* m_tasks;
  int m_taskCount;
  int m_threadCount;
};

class SampleTask : public enki::ITaskSet
{
public:
  SampleTask() = default;

  void ExecuteRange(enki::TaskSetPartition range, uint32_t threadIndex) override
  {
    m_task(range.start, range.end, threadIndex, m_taskContext);
  }

  b2TaskCallback* m_task = nullptr;
  void* m_taskContext = nullptr;
};

static void*
EnqueueTask(b2TaskCallback* task, int32_t itemCount, int32_t minRange, void* taskContext, void* userContext)
{
  Sample* sample = static_cast<Sample*>(userContext);
  if (sample->m_taskCount < Sample::m_maxTasks) {
    SampleTask& sampleTask = sample->m_tasks[sample->m_taskCount];
    sampleTask.m_SetSize = itemCount;
    sampleTask.m_MinRange = minRange;
    sampleTask.m_task = task;
    sampleTask.m_taskContext = taskContext;
    sample->m_scheduler->AddTaskSetToPipe(&sampleTask);
    ++sample->m_taskCount;
    return &sampleTask;
  } else {
    // This is not fatal but the maxTasks should be increased
    assert(false);
    task(0, itemCount, 0, taskContext);
    return nullptr;
  }
};

static void
FinishTask(void* taskPtr, void* userContext)
{
  if (taskPtr != nullptr) {
    SampleTask* sampleTask = static_cast<SampleTask*>(taskPtr);
    Sample* sample = static_cast<Sample*>(userContext);
    sample->m_scheduler->WaitforTask(sampleTask);
  }
};

// store one physics world...
static b2WorldId worldId = b2_nullWorldId;
static SampleContext m_context;
static Sample m_sample;
static b2WorldDef world_def;
static bool init = false;

void
physics_reset_task_count()
{
  if (!init)
    return;
  m_sample.m_taskCount = 0;
}

b2WorldId
emplace_or_replace_physics_world()
{
  if (!init) {
    init = true;

    // const auto logical_cpu_cores = SDL_GetNumLogicalCPUCores();
    // SDL_Log("LogicalCPUCores: %i", logical_cpu_cores);

    const int maxThreadCount = enki::GetNumHardwareThreads();
    const int half_threads = (int)(maxThreadCount * 0.5f);
    m_context.workerCount = b2ClampInt(half_threads, 1, maxThreadCount);
    SDL_Log("(box2d) workerCount: %i", m_context.workerCount);

    m_sample.m_context = &m_context;
    m_sample.m_scheduler = new enki::TaskScheduler;
    m_sample.m_scheduler->Initialize(m_context.workerCount);
    m_sample.m_tasks = new SampleTask[m_sample.m_maxTasks];
    m_sample.m_taskCount = 0;
    m_sample.m_threadCount = 1 + m_context.workerCount;

    world_def = b2DefaultWorldDef();
    world_def.gravity = { 0.0f, 0.0f };
    world_def.workerCount = m_context.workerCount;
    world_def.enqueueTask = EnqueueTask;
    world_def.finishTask = FinishTask;
    world_def.userTaskContext = &m_sample;
    world_def.enableSleep = true;
    worldId = b2CreateWorld(&world_def);
  }

  // cleanup physics world...
  static bool needs_deleting = false;
  if (needs_deleting) {
    SDL_Log("%s", "cleaning up physics world..");
    b2DestroyWorld(worldId);
    worldId = b2_nullWorldId;
    worldId = b2CreateWorld(&world_def);
  }

  needs_deleting = true;
  SDL_Log("%s", "Physics world initialized");

  return worldId;
}

void
physics_shutdown()
{
  if (!init)
    return;
  // SDL_Log("shutdown - physics_shutdown()");

  b2DestroyWorld(worldId);
  worldId = b2_nullWorldId;

  m_sample.m_scheduler->WaitforAllAndShutdown();
  delete m_sample.m_scheduler;
  m_sample.m_scheduler = nullptr;

  delete[] m_sample.m_tasks;
  m_sample.m_tasks = nullptr;

  init = false;
}

} // namespace game2d