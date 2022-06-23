#pragma once
#include <rally/application/application.h>
#include <rally/types.h>
#include <windows.h>

namespace rally {
struct Application;
constexpr u32 kMaxThreadCount = 16;
constexpr u32 kMaxJobCount = 128;
struct ThreadPool;
struct Job {
  job_func callback;
  void* data;
};
enum class PerformNextJobResponse : u32 {
  kShouldSleep = 0,
  kCompletedJob = 1,
  kFailedToSecureJob = 2,
};
struct JobQueue {
  volatile u32 front;
  volatile u32 end;
  volatile u32 completion_goal;
  volatile u32 completion_count;
  volatile b32 active;
  Job jobs[kMaxJobCount];
  HANDLE semaphore;
};
struct ThreadInfo {
  u32 thread_id;
  ThreadPool* threadpool;
};
struct ThreadPool {
  u32 thread_count;
  JobQueue* queue;
  HANDLE thread_handles[kMaxThreadCount];
  ThreadInfo thread_infos[kMaxThreadCount];
};
struct ThreadPoolCreateInfo {
  u32 thread_count;
};
bool CreateThreadPool(ThreadPoolCreateInfo* thread_pool_ci,
                      Application* thread_pool);
void PushJob(JobQueue* queue, Job job);
void PushJobs(JobQueue* queue, Job* jobs_head, u32 job_count);
void WaitThreadQueue(JobQueue* queue);
void DestroyThreadPool(ThreadPool* thread_pool);
}  // namespace rally