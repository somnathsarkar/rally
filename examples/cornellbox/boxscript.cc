#include <examples/cornellbox/boxscript.h>
#include <math.h>

enum class BoxMaterials : rally::u32 {
  kWhite = 0,
  kRed = 1,
  kGreen = 2,
  kBlue = 3,
};

bool CreateBoxScript(rally::Application* app) {
  // Setup camera
  rally::Mat4 view_to_world = rally::MTranslation(0.0f, 0.0f, -2.0f);
  rally::Mat4 view_to_projection = rally::MPerspective(
      rally::Radians(100.0f),
      (rally::r32)app->renderer->width / app->renderer->height, 0.01f, 100.0f);
  rally::Mat4 projection_to_view = rally::MInverse(view_to_projection);
  rally::Mat4 projection_to_world =
      rally::MMul(view_to_world, projection_to_view);
  app->scene->main_camera->perspective_to_world = projection_to_world;
  app->scene->main_camera->view_to_world = view_to_world;

  // Back face
  rally::Scene* scene = app->scene;
  scene->entities[0] = 0;
  scene->material_ids[0] = (rally::u32)BoxMaterials::kWhite;
  scene->entity_count = 1;
  scene->transforms[0] = rally::kIdentity;

  // Top face
  scene->entities[1] = 0;
  scene->material_ids[1] = (rally::u32)BoxMaterials::kWhite;
  scene->entity_count = 2;
  scene->transforms[1] = rally::MTranslation(0.0f, 1.0f, -1.0f);

  // Bottom face
  scene->entities[2] = 0;
  scene->material_ids[2] = (rally::u32)BoxMaterials::kWhite;
  scene->entity_count = 3;
  scene->transforms[2] = rally::MTranslation(0.0f, -1.0f, -1.0f);

  // Left face
  scene->entities[3] = 0;
  scene->material_ids[3] = (rally::u32)BoxMaterials::kRed;
  scene->entity_count = 4;
  scene->transforms[3] = rally::MTranslation(-1.0f, 0.0f, -1.0f);

  // Right face
  scene->entities[4] = 0;
  scene->material_ids[4] = (rally::u32)BoxMaterials::kGreen;
  scene->entity_count = 5;
  scene->transforms[4] = rally::MTranslation(1.0f, 0.0f, -1.0f);

  // Point light
  scene->lights[0] = {{0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 1.0f, 0.5f};
  scene->light_count = 1;

  // Sphere
  scene->entities[5] = 1;
  scene->material_ids[5] = (rally::u32)BoxMaterials::kBlue;
  scene->entity_count = 6;
  scene->transforms[5] =
      rally::MMul(rally::MTranslation(0, 0, -1.0f), rally::MScale(0.1f));
  return false;
}
bool UpdateBoxScript(rally::Application* app) {
  // TODO: Rework this with QPC time system
  static float current_time = 0.0f;
  // Update Light
  constexpr float light_speed = 0.05f;
  constexpr float light_radius = 0.33f;
  rally::Vec4 light_pos = {
      light_radius * sinf(current_time * light_speed), 0.0f,
      light_radius * cosf(current_time * light_speed), 1.0f};
  light_pos = rally::VMul(rally::MRotation(0.0f, 0.0f, rally::Radians(30.0f)),
                          light_pos);
  light_pos = rally::VMul(rally::MTranslation(0.0f, 0.0f, -1.0f), light_pos);
  (app->scene->lights[0]).position = light_pos;
  // Update time
  current_time += 1.0f;
  return false;
}