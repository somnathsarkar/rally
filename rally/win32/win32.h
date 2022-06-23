#pragma once
#include <windows.h>
#include <rally/types.h>
#include <rally/application/application.h>

namespace rally {
struct Application;
constexpr s64 kMaxWindowEvents = 128;
enum class WindowEventType : u32 {
  kUnknown = 0,
  kKeyDown = 1,
  kKeyUp = 2,
  kMax = 3,
};
enum class Key : u32 {
  Unknown = 0,
  W = 1,
  A = 2,
  S = 3,
  D = 4,
  Max = 5,
};
union EventData {
  Key key;
};
struct WindowEvent {
  WindowEventType type;
  EventData data;
};
struct Window {
  HINSTANCE instance;
  HWND hwnd;
  wchar_t title[256];
  u32 width;
  u32 height;
  bool active;
  u32 event_count;
  WindowEvent events[kMaxWindowEvents];
};
struct WindowCreateInfo {
  const wchar_t* title;
  u32 width;
  u32 height;
};
bool CreateWin32Window(WindowCreateInfo* window_ci, Application* app);
bool UpdateWin32Window(Window* window);
void DestroyWin32Window(Window* window);
}  // namespace rally