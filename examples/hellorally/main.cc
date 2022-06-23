#include <examples/hellorally/helloscript.h>
#include <rally/application/application.h>
#include <stdio.h>

using namespace rally;

int main() {
  ThreadPoolCreateInfo thread_ci{2};
  WindowCreateInfo window_ci{L"Hello rally!", 640, 480};
  RendererCreateInfo renderer_ci{RenderMode::kRasterization, 640, 480, 3, 2};
  SceneImportInfo scene_ii{true};
  ScriptCreateInfo script_ci{CreateHelloScript, UpdateHelloScript};
  ApplicationCreateInfo app_ci{&thread_ci, &window_ci, &renderer_ci, &scene_ii,
                               &script_ci};
  constexpr s64 kAppMemorySize = Gigabytes(1);
  void* app_memory = malloc(kAppMemorySize);
  Application* app = CreateApplication(&app_ci, app_memory, kAppMemorySize);
  if (app == nullptr) {
    DestroyApplication(app);
    return 0;
  }
  while (IsApplicationActive(app)) {
    UpdateApplication(app);
    for (u32 event_i = 0; event_i < (app->window->event_count); event_i++) {
      printf("%d\n", (u32)(app->window->events[event_i].data.key));
    }
  };
  DestroyApplication(app);
  return 0;
}