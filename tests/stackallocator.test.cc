#include <gtest/gtest.h>
#include <rally/memory/stackallocator.h>

TEST(StackAllocator, StackAllocate) {
  rally::s64 mem_size = rally::Megabytes(1);
  void* mem = malloc(mem_size);
  rally::StackAllocator* stack_allocator =
      rally::CreateStackAllocator(mem, mem_size);
  char* c = (char*)rally::StackAllocate(stack_allocator, sizeof(char),
                                        alignof(char));
  *c = 'A';
  struct AllocateTest {
    int a[10];
  };
  size_t occupied0 = stack_allocator->occupied;
  AllocateTest* at = (AllocateTest*)rally::StackAllocate(
      stack_allocator, sizeof(AllocateTest), alignof(AllocateTest));
  at->a[2] = 15;
  EXPECT_NE(c, nullptr);
  EXPECT_EQ(*c, 'A');
  EXPECT_NE(at, nullptr);
  EXPECT_EQ(at->a[0], 0);
  EXPECT_EQ(at->a[2], 15);
  struct AllocateFailureTest {
    int b[1000000];
  };
  AllocateFailureTest* aft = (AllocateFailureTest*)rally::StackAllocate(
      stack_allocator, sizeof(AllocateFailureTest),
      alignof(AllocateFailureTest));
  EXPECT_EQ(aft, nullptr);
  rally::DestroyStackAllocator(stack_allocator);
}