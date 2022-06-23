#pragma once
#include <stdint.h>

namespace rally {
typedef uint32_t u32;
typedef uint64_t s64;
typedef uint64_t u64;
typedef int32_t i32;
typedef int32_t b32;
typedef float r32;
typedef double r64;
typedef bool (*job_func)(void*);
constexpr r32 kPi = 3.14159265359f;
inline constexpr s64 Kilobytes(s64 x) { return x * 1024; }
inline constexpr s64 Megabytes(s64 x) { return x * 1024 * 1024; }
inline constexpr s64 Gigabytes(s64 x) { return x * 1024 * 1024 * 1024; }
inline constexpr s64 Terabytes(s64 x) { return x * 1024 * 1024 * 1024 * 1024; }
inline constexpr r32 Radians(r32 x) { return x * kPi / 180.0f; }
inline constexpr r32 Degrees(r32 x) { return x * 180.0f / kPi; }
}  // namespace rally