#pragma once
#include <rally/application/application.h>
#include <rally/scene/assets.h>
#include <rally/types.h>

namespace rally {
struct PerspectiveCamera;
struct SceneResources {
  Mesh* meshes;
  u32 mesh_count;
  u32 max_meshes;

  Vertex* vertices;
  u32 vertex_count;
  u32 max_vertices;

  Index* indices;
  u32 index_count;
  u32 max_indices;

  Material* materials;
  u32 material_count;
  u32 max_materials;
};
struct Scene {
  Mat4* transforms;
  u32* entities;
  u32* material_ids;
  PointLight* lights;
  SceneResources* resources;
  u32 entity_count;
  u32 max_entities;
  u32 light_count;
  u32 max_lights;
  PerspectiveCamera* main_camera;
};
struct SceneCreateInfo {
  u32 max_entities;
  u32 max_lights;
  u32 max_meshes;
  u32 max_vertices;
  u32 max_indices;
  u32 max_materials;
};
struct SceneImportInfo {
  b32 import_scene;
};
bool CreateScene(SceneCreateInfo* scene_ci, Application* application);
}  // namespace rally