#pragma once
#include <assert.h>
#include <windows.h>

#ifndef NDEBUG
#define ASSERT(condition, message)   \
  do {                               \
    if (!(condition)) {              \
      OutputDebugStringA("Error: "); \
      OutputDebugStringA((message)); \
      OutputDebugStringA("\n");      \
      assert(false && (message));    \
    }                                \
  } while (false)
#else
#define ASSERT(condition, message) \
  do {                             \
  } while (false)
#endif