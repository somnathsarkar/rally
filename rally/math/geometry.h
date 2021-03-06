#pragma once
#include <rally/math/vec.h>

namespace rally {
struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec3 tangent;
  Vec3 bitangent;
  Vec2 uv;
};
typedef u32 Index;
struct PointLight {
  Vec4 position;
  r32 color_r, color_g, color_b;
  r32 intensity;
};
struct PerspectiveCamera {
  Mat4 view_to_world;
  Mat4 perspective_to_world;
};
struct Material {
  r32 albedo_r, albedo_g, albedo_b;
  r32 reflectance;
  r32 metallic;
  r32 roughness;
  r32 reflectivity;
  r32 _pad;
};
struct Instance {
  i32 vertex_offset;
  i32 index_offset;
  i32 material_id;
  i32 _pad;
};
}  // namespace rally