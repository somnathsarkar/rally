#include <rally/dev/dev.h>
#include <rally/thread/threadpool.h>

namespace rally {
static void DeactivateQueue(JobQueue* queue, u32 thread_count) {
  InterlockedAnd((LONG volatile*)&queue->active, 0);
  ReleaseSemaphore(queue->semaphore, thread_count, NULL);
}
static PerformNextJobResponse PerformNextJob(JobQueue* queue,
                                             ThreadInfo* thread_info) {
  u32 original_next_job = queue->front;
  u32 original_job_increment = (original_next_job + 1) % kMaxJobCount;
  if (original_next_job != queue->end) {
    u32 job_id =
        InterlockedCompareExchange((LONG volatile*)&queue->front,
                                   original_job_increment, original_next_job);
    if (job_id == original_next_job) {
      Job job = queue->jobs[job_id];
      job.callback(job.data);
      InterlockedIncrement((LONG volatile*)&queue->completion_count);
      return PerformNextJobResponse::kCompletedJob;
    } else {
      return PerformNextJobResponse::kFailedToSecureJob;
    }
  }
  return PerformNextJobResponse::kShouldSleep;
}
static DWORD WINAPI ThreadProc(LPVOID lpParameter) {
  ThreadInfo* thread_info = (ThreadInfo*)lpParameter;
  ThreadPool* threadpool = thread_info->threadpool;
  JobQueue* queue = threadpool->queue;
  while (queue->active) {
    PerformNextJobResponse response = PerformNextJob(queue, thread_info);
    if (response == PerformNextJobResponse::kShouldSleep)
      WaitForSingleObject(queue->semaphore, INFINITE);
  }
  return 0;
}
bool CreateThreadPool(ThreadPoolCreateInfo* threadpool_ci, Application* app) {
  app->threadpool = (ThreadPool*)StackAllocate(app->alloc, sizeof(ThreadPool),
                                               alignof(ThreadPool));
  app->threadpool->queue =
      (JobQueue*)StackAllocate(app->alloc, sizeof(JobQueue), alignof(JobQueue));
  ThreadPool* threadpool = app->threadpool;
  threadpool->queue->active = true;
  threadpool->thread_count = threadpool_ci->thread_count;
  threadpool->queue->semaphore = CreateSemaphoreExW(
      NULL, 0, threadpool->thread_count, NULL, 0, SEMAPHORE_ALL_ACCESS);
  for (u32 thread_i = 0; thread_i < threadpool->thread_count; thread_i++) {
    threadpool->thread_infos[thread_i].thread_id = thread_i;
    threadpool->thread_infos[thread_i].threadpool = threadpool;
    DWORD thread_id;
    threadpool->thread_handles[thread_i] =
        CreateThread(NULL, 0, ThreadProc, &(threadpool->thread_infos[thread_i]),
                     0, &thread_id);
  }
  return false;
}
void PushJob(JobQueue* queue, Job job) {
  u32 next_queue_end = (queue->end + 1) % kMaxJobCount;
  ASSERT(next_queue_end != queue->front, "Job queue overflow!");
  queue->jobs[queue->end] = job;
  InterlockedExchange((LONG volatile*)&queue->end, next_queue_end);
  queue->completion_goal++;
  ReleaseSemaphore(queue->semaphore, 1, NULL);
}
void PushJobs(JobQueue* queue, Job* jobs_head, u32 job_count) {
  for (u32 job_i = 0; job_i < job_count; job_i++) {
    PushJob(queue, jobs_head[job_i]);
  }
}
void WaitThreadQueue(JobQueue* queue) {
  ThreadInfo main_thread_info{99, nullptr};
  while (queue->completion_count < queue->completion_goal) {
    PerformNextJob(queue, &main_thread_info);
  }
}
void DestroyThreadPool(ThreadPool* threadpool) {
  if (threadpool == nullptr) return;
  DeactivateQueue(threadpool->queue, threadpool->thread_count);
  WaitForMultipleObjects(threadpool->thread_count, threadpool->thread_handles,
                         TRUE, INFINITE);
  for (u32 thread_i = 0; thread_i < threadpool->thread_count; thread_i++) {
    CloseHandle(threadpool->thread_handles[thread_i]);
  }
}
}  // namespace rally