#include <examples/hellorally/helloscript.h>
#include <math.h>

static bool BuildScene(rally::Application* app) {
  // Setup camera
  rally::Mat4 view_to_world = rally::MTranslation(0.0f, 0.0f, -2.0f);
  rally::Mat4 view_to_projection = rally::MPerspective(
      rally::Radians(100.0f),
      (rally::r32)app->renderer->width / app->renderer->height, 0.01f, 100.0f);
  rally::Mat4 projection_to_view = MInverse(view_to_projection);
  rally::Mat4 projection_to_world = MMul(view_to_world, projection_to_view);
  app->scene->main_camera->perspective_to_world = projection_to_world;
  app->scene->main_camera->view_to_world = view_to_world;

  // Setup entity (cube)
  app->scene->entities[0] = 0;
  app->scene->material_ids[0] = 0;
  app->scene->entity_count = 1;
  app->scene->transforms[0] = rally::MRotation(0, 0, 0);

  // Setup point light
  app->scene->light_count = 1;
  app->scene->lights[0] = {{0, 0, -10.0f, 0}, 1.0f, 1.0f, 1.0f, 10.0f};
  return false;
}

bool CreateHelloScript(rally::Application* app) {
  bool failed = false;
  failed |= BuildScene(app);
  return failed;
}

bool UpdateHelloScript(rally::Application* app) {
  // Update cube rotation
  static float time_total = 0;
  app->scene->transforms[0] = rally::MRotation(0.0f, time_total, 0.0f);
  time_total += 0.01f;

  // Update light intensity
  app->scene->lights[0].intensity = fabs(sin(time_total)*10.0f);
  return false;
}