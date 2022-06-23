#include <rally/scene/scene.h>

namespace rally {
bool CreateScene(SceneCreateInfo* scene_ci, Application* application) {
  application->scene = SALLOC(application->alloc, Scene, 1);
  Scene* scene = application->scene;

  scene->max_entities = scene_ci->max_entities;
  scene->max_lights = scene_ci->max_lights;
  scene->transforms = SALLOC(application->alloc, Mat4, scene->max_entities);
  scene->entities = SALLOC(application->alloc, u32, scene->max_entities);
  scene->material_ids = SALLOC(application->alloc, u32, scene->max_entities);
  scene->lights = SALLOC(application->alloc, PointLight, scene->max_lights);
  scene->main_camera = SALLOC(application->alloc, PerspectiveCamera, 1);

  scene->resources = SALLOC(application->alloc, SceneResources, 1);
  SceneResources* res = scene->resources;
  res->max_meshes = scene_ci->max_meshes;
  res->max_vertices = scene_ci->max_vertices;
  res->max_indices = scene_ci->max_indices;
  res->max_materials = scene_ci->max_materials;
  res->meshes = SALLOC(application->alloc, Mesh, res->max_meshes);
  res->vertices = SALLOC(application->alloc, Vertex, res->max_vertices);
  res->indices = SALLOC(application->alloc, Index, res->max_indices);
  res->materials = SALLOC(application->alloc, Material, res->max_materials);
  return false;
}
}  // namespace rally