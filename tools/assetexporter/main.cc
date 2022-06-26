#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <direct.h>
#include <rally/application/application.h>
#include <rally/math/geometry.h>
#include <stdio.h>
#include <sys/stat.h>

#include <assimp/Importer.hpp>
#include <sstream>
#include <string>

using namespace rally;

#define REL_POINT(p, base, type) p = (type*)((char*)p - (char*)base)

void PreprocessModel(const char* read_buffer, SceneCreateInfo* scene_ci) {
  // TODO: Correct index processing with aiFace
  char filename_buffer[256];
  sscanf(read_buffer + 6, "%s", filename_buffer);
  Assimp::Importer* importer = new Assimp::Importer();
  std::string filepath = filename_buffer;
  importer->ReadFile(filepath, aiProcess_GenNormals | aiProcess_GenUVCoords |
                                   aiProcess_CalcTangentSpace);
  const aiScene* scene = importer->GetScene();
  u32 num_meshes = scene->mNumMeshes;
  scene_ci->max_meshes += num_meshes;
  for (u32 mesh_i = 0; mesh_i < num_meshes; mesh_i++) {
    const aiMesh* mesh = scene->mMeshes[mesh_i];
    u32 vert_count = mesh->mNumVertices;
    scene_ci->max_vertices += vert_count;
    scene_ci->max_indices += vert_count;
  }
  delete importer;
}

void ProcessModel(const char* read_buffer, Scene* scene) {
  // TODO: Correct index processing with aiFace
  char filename_buffer[256];
  sscanf(read_buffer + 6, "%s", filename_buffer);
  Assimp::Importer* importer = new Assimp::Importer();
  std::string filepath = filename_buffer;
  importer->ReadFile(filepath, aiProcess_GenNormals | aiProcess_GenUVCoords |
                                   aiProcess_CalcTangentSpace);
  const aiScene* ai_scene = importer->GetScene();
  u32 num_meshes = ai_scene->mNumMeshes;
  for (u32 mesh_i = 0; mesh_i < num_meshes; mesh_i++) {
    const aiMesh* mesh = ai_scene->mMeshes[mesh_i];
    u32 vert_count = mesh->mNumVertices;
    u32 vert_offset = scene->resources->vertex_count;
    u32 index_offset = scene->resources->index_count;
    for (u32 vert_i = 0; vert_i < vert_count; vert_i++) {
      const aiVector3D ai_vertex = mesh->mVertices[vert_i];
      const aiVector3D ai_normal = mesh->mNormals[vert_i];
      const aiVector3D ai_tan = mesh->mTangents[vert_i];
      const aiVector3D ai_bitan = mesh->mBitangents[vert_i];
      const aiVector3D ai_uv = mesh->mTextureCoords[0][vert_i];
      Vertex vert{{ai_vertex.x, ai_vertex.y, ai_vertex.z},
                  {ai_normal.x, ai_normal.y, ai_normal.z},
                  {ai_tan.x, ai_tan.y, ai_tan.z},
                  {ai_bitan.x, ai_bitan.y, ai_bitan.z},
                  {ai_uv.x, ai_uv.y}};
      scene->resources->vertices[vert_i + vert_offset] = vert;
      scene->resources->indices[vert_i + index_offset] = vert_i;
    }
    scene->resources->vertex_count += vert_count;
    scene->resources->index_count += vert_count;
    scene->resources->mesh_count++;
  }
  delete importer;
}

void PreprocessMaterial(char* read_buffer, SceneCreateInfo* scene_ci) {
  scene_ci->max_materials++;
}

void ProcessMaterial(char* read_buffer, Scene* scene) {
  u32 mat_i = scene->resources->material_count;
  Material* mat = &scene->resources->materials[mat_i];
  sscanf(read_buffer + 9, "%f %f %f %f", &mat->albedo_r, &mat->albedo_g,
         &mat->albedo_b, &mat->shininess);
  scene->resources->material_count++;
}

int main(int argc, char* argv[]) {
  // Start empty application
  s64 app_size = rally::Megabytes(128);
  void* app_mem = malloc(app_size);
  rally::ApplicationCreateInfo app_ci{0};
  Application* app = CreateApplication(&app_ci, app_mem, app_size);

  // Initialize preprocess resources
  // TODO: Import these from assets.txt
  SceneCreateInfo scene_ci = {0};
  scene_ci.max_entities = 5;
  scene_ci.max_lights = 1;

  // Preprocess: Compute size of resources
  const char* read_filepath = "assets.txt";
  FILE* manifest_file = fopen(read_filepath, "r");
  char read_buffer[256];
  while (fgets(read_buffer, 256, manifest_file)) {
    if (strncmp(read_buffer, "MODEL", 5) == 0) {
      PreprocessModel(read_buffer, &scene_ci);
    } else if (strncmp(read_buffer, "MATERIAL", 8) == 0) {
      PreprocessMaterial(read_buffer, &scene_ci);
    } else {
      printf("Failed to preprocess asset: %s\n", read_buffer);
    }
  }

  // Create Scene
  CreateScene(&scene_ci, app);
  s64 occupied = app->alloc->occupied;
  s64 data_size = ((char*)app->alloc->data + occupied) - ((char*)app->scene);

  // Process: Populate Scene
  fseek(manifest_file, 0, SEEK_SET);
  while (fgets(read_buffer, 256, manifest_file)) {
    if (strncmp(read_buffer, "MODEL", 5) == 0) {
      ProcessModel(read_buffer, app->scene);
    } else if (strncmp(read_buffer, "MATERIAL", 8) == 0) {
      ProcessMaterial(read_buffer, app->scene);
    } else {
      printf("Failed to process asset: %s\n", read_buffer);
    }
  }
  fclose(manifest_file);

  // Make pointers relative: No more modifications after this point!
  rally::Scene* sp = app->scene;
  rally::SceneResources* sr = sp->resources;
  REL_POINT(sr->meshes, sp, Mesh);
  REL_POINT(sr->vertices, sp, Vertex);
  REL_POINT(sr->indices, sp, Index);
  REL_POINT(sr->materials, sp, Material);
  REL_POINT(sp->resources, sp, SceneResources);
  REL_POINT(sp->transforms, sp, Mat4);
  REL_POINT(sp->entities, sp, u32);
  REL_POINT(sp->material_ids, sp, u32);
  REL_POINT(sp->lights, sp, PointLight);
  REL_POINT(sp->main_camera, sp, PerspectiveCamera);

  // Create write filepath if it does not exist
  struct _stat64i32 stat_buff;
  if (_stat64i32(argv[1], &stat_buff) == -1) {
    _mkdir(argv[1]);
  }

  // Write scene to binary file
  std::string write_filepath = argv[1];
  write_filepath += "/assets.bin";
  FILE* wf = fopen(write_filepath.c_str(), "wb");
  fwrite(sp, data_size, 1, wf);
  fclose(wf);

  // Cleanup
  DestroyApplication(app);
  delete app_mem;
  return 0;
}