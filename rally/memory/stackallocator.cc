#include <rally/memory/stackallocator.h>
#include <string.h>

namespace rally {
StackAllocator* CreateStackAllocator(void* data, s64 data_size) {
  memset(data, 0, data_size);
  StackAllocator* alloc = (StackAllocator*)data;
  alloc->size = data_size;
  alloc->occupied = sizeof(StackAllocator);
  alloc->data = data;
  return alloc;
}
void* StackAllocate(StackAllocator* stack_alloc, s64 alloc_size,
                    s64 alloc_align) {
  return StackAllocateArray(stack_alloc, 1, alloc_size, alloc_align);
}
void* StackAllocateArray(StackAllocator* stack_alloc, s64 array_len,
                         s64 alloc_size, s64 alloc_align) {
  s64 alloc_blocks = (stack_alloc->occupied + alloc_align - 1) / alloc_align;
  s64 begin_alloc = alloc_blocks * alloc_align;
  s64 end_data = begin_alloc + alloc_size * array_len;
  s64 mark_block = (end_data + alignof(s64) - 1) / alignof(s64);
  s64 mark = mark_block * alignof(s64);
  s64 end_alloc = mark + sizeof(s64);
  if (end_alloc > stack_alloc->size) return nullptr;
  // Write marker
  s64* marker = (s64*)((char*)stack_alloc->data + mark);
  *marker = stack_alloc->occupied;
  stack_alloc->occupied = end_alloc;
  return (char*)stack_alloc->data + begin_alloc;
}
s64 StackFree(StackAllocator* stack_alloc) {
  if (stack_alloc->occupied == 0) return 0;
  s64* mark =
      (s64*)((char*)stack_alloc->data + stack_alloc->occupied - sizeof(s64));
  s64 old_occupied = stack_alloc->occupied;
  stack_alloc->occupied = *mark;
  s64 free_size = old_occupied - stack_alloc->occupied;
  memset((char*)stack_alloc->data + stack_alloc->occupied, 0, free_size);
  return free_size;
}
void DestroyStackAllocator(StackAllocator* stack_alloc) {
  // Do nothing, we don't manage our own mememory
}
}  // namespace rally