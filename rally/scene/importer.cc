#include <rally/dev/dev.h>
#include <rally/scene/importer.h>

#define FIXUP_POINT(p, base, type) p = (type*)((s64)p + (s64)base)

namespace rally {
bool ImportScene(Application* app) {
  HANDLE asset_file =
      CreateFileA("assets.bin", GENERIC_READ, FILE_SHARE_READ, NULL,
                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (asset_file == INVALID_HANDLE_VALUE) {
    DWORD last_error = GetLastError();
    ASSERT(asset_file != INVALID_HANDLE_VALUE, "File not found!");
  }
  DWORD asset_file_size = GetFileSize(asset_file, NULL);
  app->scene =
      (Scene*)StackAllocate(app->alloc, asset_file_size, alignof(Scene));
  DWORD bytes_read = 0;
  BOOL read_success =
      ReadFile(asset_file, app->scene, asset_file_size, &bytes_read, NULL);
  ASSERT(bytes_read == asset_file_size, "File size mismatch!");
  CloseHandle(asset_file);

  // Fixup pointers
  Scene* sp = app->scene;
  FIXUP_POINT(sp->main_camera, sp, PerspectiveCamera);
  FIXUP_POINT(sp->lights, sp, PointLight);
  FIXUP_POINT(sp->material_ids, sp, u32);
  FIXUP_POINT(sp->entities, sp, u32);
  FIXUP_POINT(sp->transforms, sp, Mat4);
  FIXUP_POINT(sp->resources, sp, SceneResources);
  SceneResources* sr = sp->resources;
  FIXUP_POINT(sr->meshes, sp, Mesh);
  FIXUP_POINT(sr->vertices, sp, Vertex);
  FIXUP_POINT(sr->indices, sp, Index);
  FIXUP_POINT(sr->materials, sp, Material);

  return false;
}
}  // namespace rally