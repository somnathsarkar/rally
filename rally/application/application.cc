#include <rally/application/application.h>
#include <rally/scene/importer.h>

namespace rally {
Application* CreateApplication(ApplicationCreateInfo* app_ci, void* data,
                               s64 data_size) {
  // Create allocator and store application data
  StackAllocator* stack_alloc = CreateStackAllocator(data, data_size);
  Application* app = (Application*)StackAllocate(
      stack_alloc, sizeof(Application), alignof(Application));
  app->alloc = stack_alloc;
  bool failed = false;

  // Create Thread Pool
  if (app_ci->thread_ci != nullptr)
    failed |= CreateThreadPool(app_ci->thread_ci, app);
  if (failed) return nullptr;

  // Import scene at assets.bin
  if (app_ci->scene_ii != nullptr) failed |= ImportScene(app);
  if (failed) return nullptr;

  // Create Win32 Window
  if (app_ci->window_ci != nullptr)
    failed |= CreateWin32Window(app_ci->window_ci, app);
  if (failed) return nullptr;

  // Create Renderer
  if (app_ci->render_ci != nullptr)
    failed |= CreateRenderer(app_ci->render_ci, app);
  if (failed) return nullptr;

  // Create script object and run script create function
  failed |= CreateScript(app_ci->script_ci, app);
  if (failed) return nullptr;
  return app;
}
bool UpdateApplication(Application* app) {
  UpdateWin32Window(app->window);
  if (app->script->update_func != nullptr) app->script->update_func(app);
  UpdateRenderer(app);
  return true;
}
bool IsApplicationActive(Application* app) { return app->window->active; }
void DestroyApplication(Application* app) {
  DestroyThreadPool(app->threadpool);
  DestroyRenderer(app);
  DestroyWin32Window(app->window);
}
}  // namespace rally