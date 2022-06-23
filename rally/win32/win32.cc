#include <rally/win32/win32.h>
#include <stdio.h>
#include <rally/memory/stackallocator.h>

namespace rally {
static void ProcessKeyboardEvent(Window* window, LPARAM lParam, WPARAM wParam) {
  if (window->event_count >= kMaxWindowEvents) return;
  WORD key_flags = HIWORD(lParam);
  BOOL up_flag = (key_flags & KF_UP) == KF_UP;
  Key key = Key::Unknown;
  switch ((wchar_t)wParam) {
    case L'w': {
      key = Key::W;
      break;
    }
    case L'a': {
      key = Key::A;
      break;
    }
    case L's': {
      key = Key::S;
      break;
    }
    case L'd': {
      key = Key::D;
      break;
    }
    default: {
      return;
    }
  }
  window->events[window->event_count].type =
      (up_flag) ? WindowEventType::kKeyUp : WindowEventType::kKeyDown;
  window->events[window->event_count].data.key = key;
  window->event_count++;
}
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam) {
  Window* window = (Window*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
  switch (uMsg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
    case WM_CLOSE:
      window->active = false;
      return 0;
    case WM_KEYUP: {
      switch (wParam) {
        case VK_ESCAPE: {
          window->active = false;
          return 0;
        }
      }
      break;
    }
    case WM_CHAR: {
      ProcessKeyboardEvent(window, lParam, wParam);
      return 0;
    }
  }
  return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
bool CreateWin32Window(WindowCreateInfo* window_ci, Application* app) {
  Window* window =
      (Window*)StackAllocate(app->alloc, sizeof(Window), alignof(Window));
  app->window = window;
  HINSTANCE instance = GetModuleHandleW(NULL);

  WNDCLASSEXW wnd{};
  wnd.cbSize = sizeof(WNDCLASSEXW);
  wnd.style = 0;
  wnd.lpfnWndProc = WindowProc;
  wnd.cbClsExtra = 0;
  wnd.cbWndExtra = 0;
  wnd.hInstance = instance;
  wnd.hIcon = NULL;
  wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
  wnd.hbrBackground = NULL;
  wnd.lpszMenuName = NULL;
  wnd.lpszClassName = window_ci->title;
  wnd.hIconSm = NULL;
  RegisterClassExW(&wnd);

  HWND hwnd = CreateWindowExW(0, window_ci->title, window_ci->title, 0, 0, 0,
                              window_ci->width, window_ci->height, NULL, NULL,
                              instance, NULL);
  window->hwnd = hwnd;
  window->instance = instance;
  wcscpy(window->title, window_ci->title);
  window->width = window_ci->width;
  window->height = window_ci->height;
  window->active = true;
  SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
  ShowWindow(hwnd, SW_SHOW);
  return hwnd == NULL;
}
bool UpdateWin32Window(Window* window) {
  window->event_count = 0;
  MSG msg{};
  while (PeekMessageW(&msg, window->hwnd, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return true;
}
void DestroyWin32Window(Window* window) {
  if(window==nullptr) return;
  if (window->hwnd) {
    DestroyWindow(window->hwnd);
  }
}
}  // namespace rally