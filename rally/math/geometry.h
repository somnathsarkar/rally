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
  r32 shininess;
};
}  // namespace rally