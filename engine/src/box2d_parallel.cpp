#include "core/pch.hpp"

#include "box2d_parallel.hpp"

namespace game2d {

void*
EnqueueTask(b2TaskCallback* task, int32_t itemCount, int32_t minRange, void* taskContext, void* userContext)
{
  Sample* sample = static_cast<Sample*>(userContext);
  if (sample->m_taskCount < maxTasks) {
    SampleTask& sampleTask = sample->m_tasks[sample->m_taskCount];
    sampleTask.m_SetSize = itemCount;
    sampleTask.m_MinRange = minRange;
    sampleTask.m_task = task;
    sampleTask.m_taskContext = taskContext;
    sample->m_scheduler.AddTaskSetToPipe(&sampleTask);
    ++sample->m_taskCount;
    return &sampleTask;
  } else {
    // This is not fatal but the maxTasks should be increased
    assert(false);
    task(0, itemCount, 0, taskContext);
    return nullptr;
  }
};

void
FinishTask(void* taskPtr, void* userContext)
{
  if (taskPtr != nullptr) {
    SampleTask* sampleTask = static_cast<SampleTask*>(taskPtr);
    Sample* sample = static_cast<Sample*>(userContext);
    sample->m_scheduler.WaitforTask(sampleTask);
  }
};

} // namespace game2d