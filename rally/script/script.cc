#include <rally/script/script.h>

namespace rally {
bool CreateScript(ScriptCreateInfo* script_ci, Application* app) {
  app->script = SALLOC(app->alloc, Script, 1);
  if (script_ci == nullptr) return false;
  app->script->create_func = script_ci->create_func;
  app->script->update_func = script_ci->update_func;
  app->script->create_func(app);
  return false;
}
}  // namespace rally