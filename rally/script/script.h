#pragma once
#include <rally/types.h>
#include <rally/application/application.h>

namespace rally{
struct ScriptCreateInfo{
  bool (*create_func)(Application*);
  bool (*update_func)(Application*);
};
struct Script{
  bool (*create_func)(Application*);
  bool (*update_func)(Application*);
};
bool CreateScript(ScriptCreateInfo* script_ci, Application* app);
}