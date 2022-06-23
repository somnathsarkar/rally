add_library(
  rally
  win32/win32.cc
  memory/stackallocator.cc
  application/application.cc
  render/renderer.cc
  thread/threadpool.cc
  math/vec.cc
  scene/scene.cc
  scene/importer.cc
  script/script.cc
)

target_include_directories(rally PUBLIC ${CMAKE_SOURCE_DIR})
target_link_libraries(rally PUBLIC DXGI.lib D3D12.lib DXGUID.lib)

set(DXC ${CMAKE_SOURCE_DIR}/external/dxc_2021_12_08/bin/x64/dxc.exe)
if(EXISTS ${DXC})
  message("Found DirectX compiler ${DXC}, recompiling shaders")
  add_custom_target(rally_shaders DXC ${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/shader.hlsl -Fh ${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/shader.hlsl.h -T lib_6_3 -Vn g_pRaytracing -Zi -Qembed_debug)
  add_dependencies(rally rally_shaders)
else()
  message("DirectX compiler not found at ${DXC}, skipping shader recompilation")
endif()