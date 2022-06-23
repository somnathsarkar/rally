#pragma once
#include <rally/memory/stackallocator.h>
#include <rally/render/renderer.h>
#include <rally/thread/threadpool.h>
#include <rally/win32/win32.h>
#include <rally/scene/scene.h>
#include <rally/script/script.h>

namespace rally {
struct StackAllocator;
struct ThreadPool;
struct Window;
struct Renderer;
struct Scene;
struct Script;
struct ThreadPoolCreateInfo;
struct WindowCreateInfo;
struct RendererCreateInfo;
struct SceneImportInfo;
struct ScriptCreateInfo;
struct Application {
  StackAllocator* alloc;
  ThreadPool* threadpool;
  Window* window;
  Renderer* renderer;
  Scene* scene;
  Script* script;
};
struct ApplicationCreateInfo {
  ThreadPoolCreateInfo* thread_ci;
  WindowCreateInfo* window_ci;
  RendererCreateInfo* render_ci;
  SceneImportInfo* scene_ii;
  ScriptCreateInfo* script_ci;
};
Application* CreateApplication(ApplicationCreateInfo* app_ci, void* data,
                               s64 data_size);
bool UpdateApplication(Application* app);
bool IsApplicationActive(Application* app);
void DestroyApplication(Application* app);
}  // namespace rally