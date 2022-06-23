#include <gtest/gtest.h>
#include <rally/thread/threadpool.h>

using namespace rally;

struct SetArrayParams {
  u32* arr;
  u32 pos;
};

bool SetArray(SetArrayParams* params) {
  *(params->arr + params->pos) = params->pos;
  return false;
}

TEST(ThreadPool, CreateThreadPool) {
  s64 data_size = Megabytes(1);
  void* data = malloc(data_size);
  ThreadPoolCreateInfo tp_ci{8};
  ApplicationCreateInfo app_ci{&tp_ci, nullptr, nullptr};
  Application* app = CreateApplication(&app_ci, data, data_size);
  CreateThreadPool(&tp_ci, app);
  ThreadPool* tp = app->threadpool;
  EXPECT_EQ(tp->thread_count, tp_ci.thread_count);
  EXPECT_EQ(tp->queue->front, 0);
  EXPECT_EQ(tp->queue->end, 0);
  EXPECT_EQ(tp->queue->completion_count, 0);
  EXPECT_EQ(tp->queue->completion_goal,0);
  EXPECT_EQ(tp->queue->active, true);
  WaitThreadQueue(tp->queue);
  DestroyThreadPool(tp);
  free(data);
}

TEST(ThreadPool, SetArrayJob) {
  s64 data_size = Megabytes(10);
  void* data = malloc(data_size);
  ThreadPoolCreateInfo tp_ci{7};
  ApplicationCreateInfo app_ci{&tp_ci, nullptr, nullptr};
  Application* app = CreateApplication(&app_ci, data, data_size);
  ThreadPool* tp = app->threadpool;
  s64 array_len = 100;
  Job* jobs = (Job*)StackAllocateArray(app->alloc, array_len, sizeof(Job),
                                       alignof(Job));
  u32* arr = (u32*)StackAllocateArray(app->alloc, array_len, sizeof(u32),
                                      alignof(u32));
  SetArrayParams* params = (SetArrayParams*)StackAllocateArray(
      app->alloc, array_len, sizeof(SetArrayParams), alignof(SetArrayParams));
  u32 iters = 100;
  while(iters--){
    for (u32 job_i = 0; job_i < array_len; job_i++) {
      params[job_i] = {arr, job_i};
      jobs[job_i] = {(bool (*)(void*))SetArray, &params[job_i]};
      PushJob(tp->queue, jobs[job_i]);
    }
    WaitThreadQueue(tp->queue);
    for(u32 arr_i = 0; arr_i<array_len; arr_i++){
      EXPECT_EQ(arr[arr_i],arr_i);
    }
  }
  DestroyThreadPool(tp);
  free(data);
}