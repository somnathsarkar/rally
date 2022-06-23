#pragma once

#include <rally/types.h>

#define SALLOC(alloc, type, array_len) \
  (type*)StackAllocateArray((alloc), (array_len), sizeof(type), alignof(type))

namespace rally {
struct StackAllocator {
  s64 size;
  s64 occupied;
  void* data;
};
StackAllocator* CreateStackAllocator(void* data, s64 data_size);
void* StackAllocate(StackAllocator* stack_alloc, s64 alloc_size,
                    s64 alloc_align);
void* StackAllocateArray(StackAllocator* stack_alloc, s64 array_len,
                         s64 alloc_size, s64 alloc_align);
s64 StackFree(StackAllocator* stack_alloc);
void DestroyStackAllocator(StackAllocator* stack_alloc);
}  // namespace rally