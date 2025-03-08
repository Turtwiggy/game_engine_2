#pragma once

#include "common.hpp"

#include "TaskScheduler.h"
#include "box2d/box2d.h"

#include <cstdint>

namespace game2d {

// todo: work out what good values for these are
constexpr uint32_t maxTasks = 16;

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

struct Sample
{
  enki::TaskScheduler m_scheduler;
  SampleTask m_tasks[maxTasks];
  int32_t m_taskCount = 0;
};

void*
EnqueueTask(b2TaskCallback* task, int32_t itemCount, int32_t minRange, void* taskContext, void* userContext);

void
FinishTask(void* taskPtr, void* userContext);

} // namespace game2d