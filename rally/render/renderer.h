#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <rally/application/application.h>
#include <rally/math/geometry.h>
#include <rally/types.h>

namespace rally {
struct Application;
constexpr u32 kMaxFrameCount = 6;
constexpr u32 kMaxRenderThreads = 2;
constexpr u32 kMaxPointLights = 10;
struct Viewport {
  float left;
  float top;
  float right;
  float bottom;
};
struct RaygenConstant {
  Viewport viewport;
  PerspectiveCamera camera;
};
struct HitGroupConstant {
  PointLight point_lights[kMaxPointLights];
  int point_light_count;
};
struct FormatLibrary {
  DXGI_FORMAT swapchain;
  DXGI_FORMAT depth;
  DXGI_FORMAT texture;
  DXGI_FORMAT index;
  DXGI_FORMAT vertex_position;
};
struct Renderer {
  u32 width;
  u32 height;
  u32 frame_count;
  u32 thread_count;
  FormatLibrary format_library;
  IDXGIFactory2* factory;
  ID3D12Device5* device;

  // Command Lists, Queues, Allocators
  ID3D12CommandQueue* command_queue;
  ID3D12CommandAllocator* command_allocs[kMaxFrameCount][kMaxRenderThreads];
  ID3D12GraphicsCommandList6* command_lists[kMaxFrameCount][kMaxRenderThreads];

  // Synchronization values
  ID3D12Fence* fences[kMaxFrameCount][kMaxRenderThreads];
  u32 fence_values[kMaxFrameCount][kMaxRenderThreads];
  HANDLE fence_events[kMaxFrameCount][kMaxRenderThreads];

  // Swapchain assets - Resolution dependent
  IDXGISwapChain3* swapchain;
  ID3D12DescriptorHeap* rtv_descriptor_heap;
  ID3D12Resource* swapchain_buffers[kMaxFrameCount];

  // Raytracing assets
  ID3D12DescriptorHeap* descriptor_heap;
  ID3D12Resource* rt_output_buffers[kMaxFrameCount];
  ID3D12StateObject* rt_pipeline;
  ID3D12Resource* rt_tlas[kMaxFrameCount];
  ID3D12Resource* rt_blas;
  ID3D12Resource* rt_raygen_shader_table[kMaxFrameCount];
  ID3D12Resource* rt_miss_shader_table;
  ID3D12Resource* rt_hitgroup_shader_table[kMaxFrameCount];
  ID3D12Resource* rt_raygen_constant_buffer[kMaxFrameCount];
  ID3D12Resource* rt_hitgroup_constant_buffer[kMaxFrameCount];
  ID3D12RootSignature* global_signature;
  ID3D12RootSignature* raygen_local_signature;
  ID3D12RootSignature* hitgroup_local_signature;

  // Scene instance data
  D3D12_RAYTRACING_INSTANCE_DESC* rt_instances[kMaxFrameCount];

  // Scene Data
  ID3D12Resource* vertex_buffer;
  ID3D12Resource* index_buffer;
  ID3D12Resource* material_buffer;

  // Temporary Intermediate Buffers
  // TODO: Merge into singular buffer
  ID3D12Resource* as_scratch_buffer[kMaxFrameCount];
  ID3D12Resource* as_instance_buffer[kMaxFrameCount];
};
enum class RenderMode : u32 {
  kRasterization = 0,
  kRaytracing = 1,
};
struct RendererCreateInfo {
  RenderMode render_mode;
  u32 width;
  u32 height;
  u32 frame_count;
  u32 thread_count;
};
bool CreateRenderer(RendererCreateInfo* renderer_ci, Application* app);
void UpdateRenderer(Application* renderer);
void DestroyRenderer(Renderer* renderer);
}  // namespace rally