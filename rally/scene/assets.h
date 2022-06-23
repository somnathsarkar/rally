#include <rally/types.h>
#include <rally/math/geometry.h>

namespace rally{
struct Mesh{
  u32 vertex_offset;
  u32 vertex_count;
  u32 index_offset;
  u32 index_count;
};
}