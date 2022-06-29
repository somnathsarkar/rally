#include <dxgidebug.h>
#include <rally/dev/dev.h>
#include <rally/external/d3dx12.h>
#include <rally/math/geometry.h>
#include <rally/render/renderer.h>
#include <rally/render/shaders/shader.hlsl.h>

#ifndef NDEBUG
#define RENDER_DEBUG
#endif

#define DXCHECK(hr)                    \
  do {                                 \
    if (FAILED(hr)) {                  \
      ASSERT(false, "DirectX Error!"); \
      return true;                     \
    }                                  \
  } while (false)

#ifdef RENDER_DEBUG
#define DXCHECKM(hr, msg)      \
  do {                         \
    if (FAILED(hr)) {          \
      OutputDebugStringA(msg); \
      return true;             \
    }                          \
  } while (false)
#else
#define DXCHECKM(hr, msg) DXCHECK(hr)
#endif

#define DXRELEASE(resource) \
  if ((resource)) {         \
    (resource)->Release();  \
    (resource) = nullptr;   \
  }

namespace rally {
// Select GPU
static void GetGpuAdapter(IDXGIFactory2* factory, IDXGIAdapter1** adapter) {
  *adapter = nullptr;
  for (UINT adapterIndex = 0;; ++adapterIndex) {
    IDXGIAdapter1* pAdapter = nullptr;
    if (DXGI_ERROR_NOT_FOUND ==
        factory->EnumAdapters1(adapterIndex, &pAdapter)) {
      break;
    }
    // TODO: Improve device selection process
    *adapter = pAdapter;
    return;
  }
}

// Create Swapchain
static bool CreateSwapchain(Application* app) {
  Renderer* renderer = app->renderer;
  DXGI_SAMPLE_DESC sample_desc{};
  sample_desc.Count = 1;
  sample_desc.Quality = 0;

  DXGI_SWAP_CHAIN_DESC1 swapchain_desc{};
  swapchain_desc.Width = renderer->width;
  swapchain_desc.Height = renderer->height;
  swapchain_desc.Format = renderer->format_library.swapchain;
  swapchain_desc.Stereo = FALSE;
  swapchain_desc.SampleDesc = sample_desc;
  swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchain_desc.BufferCount = renderer->frame_count;
  swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
  swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swapchain_desc.Flags = 0;

  IDXGISwapChain1* swapchain;
  HRESULT hr = renderer->factory->CreateSwapChainForHwnd(
      renderer->command_queue, app->window->hwnd, &swapchain_desc, nullptr,
      nullptr, &swapchain);
  DXCHECK(hr);
  hr = swapchain->QueryInterface(IID_PPV_ARGS(&renderer->swapchain));
  DXCHECKM(hr, "Failed to upgrade swapchain interface!");
  swapchain->Release();
  hr = renderer->factory->MakeWindowAssociation(app->window->hwnd,
                                                DXGI_MWA_NO_ALT_ENTER);
  DXCHECK(hr);

  // Create descriptor heap for render target views
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  heap_desc.NumDescriptors = renderer->frame_count;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  heap_desc.NodeMask = 0;
  hr = renderer->device->CreateDescriptorHeap(
      &heap_desc, IID_PPV_ARGS(&(renderer->rtv_descriptor_heap)));
  DXCHECKM(hr, "Failed to create descriptor heap!");
  UINT rtv_heap_size = renderer->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  // Create resources for those render target views
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
      renderer->rtv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
  for (UINT frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    hr = renderer->swapchain->GetBuffer(
        frame_i, IID_PPV_ARGS(&(renderer->swapchain_buffers[frame_i])));
    DXCHECKM(hr, "Failed to get swapchain buffer!");
    renderer->device->CreateRenderTargetView(
        renderer->swapchain_buffers[frame_i], nullptr, cpu_handle);
    cpu_handle.Offset(1, rtv_heap_size);
  }
  return false;
}

// Figure out the best formats for every type of resource
static bool CreateFormatLibrary(Renderer* renderer) {
  // TODO: Add checks with multiple options for formats
  renderer->format_library.swapchain = DXGI_FORMAT_R8G8B8A8_UNORM;
  renderer->format_library.depth = DXGI_FORMAT_D24_UNORM_S8_UINT;
  renderer->format_library.texture = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  renderer->format_library.index = DXGI_FORMAT_R32_UINT;
  renderer->format_library.vertex_position = DXGI_FORMAT_R32G32B32_FLOAT;
  return false;
}

// Create single command queue
static bool CreateCommandQueue(Renderer* renderer) {
  // Create Command Queue
  D3D12_COMMAND_QUEUE_DESC command_queue_desc{};
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  command_queue_desc.NodeMask = 0;
  HRESULT hr = renderer->device->CreateCommandQueue(
      &command_queue_desc, IID_PPV_ARGS(&(renderer->command_queue)));
  DXCHECKM(hr, "Failed to create command queue!");
  return false;
}

// Create command allocators: 1 per render thread per frame
static bool CreateCommandAllocators(Renderer* renderer) {
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
      HRESULT hr = renderer->device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          IID_PPV_ARGS(&renderer->command_allocs[frame_i][thread_i]));
      DXCHECKM(hr, "Failed to create command allocator!");
    }
  }
  return false;
}

// Create command lists: 1 per render thread per frame
static bool CreateCommandLists(Renderer* renderer) {
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
      HRESULT hr = renderer->device->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT,
          renderer->command_allocs[frame_i][thread_i], nullptr,
          IID_PPV_ARGS(&renderer->command_lists[frame_i][thread_i]));
      DXCHECKM(hr, "Failed to create command list!");
      renderer->command_lists[frame_i][thread_i]->Close();
    }
  }
  return false;
}

// Create fences: 1 per render thread per frame
static bool CreateFences(Renderer* renderer) {
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
      HRESULT hr = renderer->device->CreateFence(
          0, D3D12_FENCE_FLAG_NONE,
          IID_PPV_ARGS(&renderer->fences[frame_i][thread_i]));
      renderer->fence_values[frame_i][thread_i] = 0;
      DXCHECKM(hr, "Failed to create fence!");
      renderer->fence_events[frame_i][thread_i] =
          CreateEvent(NULL, FALSE, FALSE, nullptr);
      ASSERT(renderer->fence_events[frame_i][thread_i] != nullptr,
             "Failed to create fence event!");
      if (renderer->fence_events[frame_i][thread_i] == nullptr) return true;
    }
  }
  return false;
}

// Root signature creation utility function
static bool SerializeAndCreateRootSignature(
    Renderer* renderer, D3D12_ROOT_SIGNATURE_DESC root_desc,
    ID3D12RootSignature** root_signature) {
  ID3DBlob* signature;
  ID3DBlob* error;
  HRESULT hr = D3D12SerializeRootSignature(
      &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
  DXCHECKM(hr, "Failed to serialize root signature!");
  hr = renderer->device->CreateRootSignature(0, signature->GetBufferPointer(),
                                             signature->GetBufferSize(),
                                             IID_PPV_ARGS(root_signature));
  DXCHECKM(hr, "Failed to create root signature!");
  return false;
}

// Create Root Signature
static bool CreateRootSignatures(Renderer* renderer) {
  // Global Root Signature
  {
    CD3DX12_DESCRIPTOR_RANGE uav_desc{};
    uav_desc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_DESCRIPTOR_RANGE geometry_srv_desc{};
    geometry_srv_desc.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1);
    CD3DX12_ROOT_PARAMETER root_params[3];
    root_params[0].InitAsDescriptorTable(1, &uav_desc);
    root_params[1].InitAsShaderResourceView(0);
    root_params[2].InitAsDescriptorTable(1, &geometry_srv_desc);
    CD3DX12_ROOT_SIGNATURE_DESC root_desc(3, root_params);
    SerializeAndCreateRootSignature(renderer, root_desc,
                                    &renderer->global_signature);
  }

  // Local Root Signature for Raygen Shader
  {
    CD3DX12_ROOT_PARAMETER root_params[1];
    root_params[0].InitAsConstantBufferView(0);
    CD3DX12_ROOT_SIGNATURE_DESC root_desc(1, root_params);
    root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    SerializeAndCreateRootSignature(renderer, root_desc,
                                    &renderer->raygen_local_signature);
  }

  // Local Root Signature for Hitgroup Shader
  {
    CD3DX12_DESCRIPTOR_RANGE desc_range[2];
    desc_range[0] =
        CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
    desc_range[1] =
        CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);
    CD3DX12_ROOT_PARAMETER root_params[1];
    root_params[0].InitAsDescriptorTable(2, desc_range);
    CD3DX12_ROOT_SIGNATURE_DESC root_desc(1, root_params);
    root_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    SerializeAndCreateRootSignature(renderer, root_desc,
                                    &renderer->hitgroup_local_signature);
  }
  return false;
}

// Create descriptor heap. Modify this when you need more resources
// [0,Frame_Count) = Raytracing output UAVs
// [Frame_Count,Frame_Count+3) = Geometry SRVs (Vertex,Index,Material)
// [Frame_Count+3,Frame_Count*3+3) = Hitgroup DT (CBV, Instance SRV)
static bool CreateDescriptorHeaps(Renderer* renderer) {
  // Create descriptor heap for CBV, SRV, UAV types
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc{};
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = renderer->frame_count * 3 + 3;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heap_desc.NodeMask = 0;
  HRESULT hr = renderer->device->CreateDescriptorHeap(
      &heap_desc, IID_PPV_ARGS(&(renderer->descriptor_heap)));
  DXCHECKM(hr, "Failed to create descriptor heap!");
  return false;
}

// Create resources and descriptors for Raytracing output
static bool CreateRaytracingOutputBuffers(Renderer* renderer) {
  UINT heap_size = renderer->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  // Create resources and render target views
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
      renderer->descriptor_heap->GetCPUDescriptorHandleForHeapStart());
  CD3DX12_HEAP_PROPERTIES rt_props(D3D12_HEAP_TYPE_DEFAULT);
  CD3DX12_RESOURCE_DESC rt_desc = CD3DX12_RESOURCE_DESC::Tex2D(
      renderer->format_library.swapchain, renderer->width, renderer->height, 1,
      1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  float clear_values[] = {0.0f, 0.0f, 0.0f, 1.0f};
  CD3DX12_CLEAR_VALUE rt_clear(renderer->format_library.swapchain,
                               clear_values);
  for (UINT frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    HRESULT hr = renderer->device->CreateCommittedResource(
        &rt_props, D3D12_HEAP_FLAG_NONE, &rt_desc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr,
        IID_PPV_ARGS(&(renderer->rt_output_buffers[frame_i])));
    DXCHECKM(hr, "Failed to create Raytracing output buffer!");
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc{};
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    renderer->device->CreateUnorderedAccessView(
        renderer->rt_output_buffers[frame_i], nullptr, &uav_desc, cpu_handle);
    cpu_handle.Offset(1, heap_size);
  }
  return false;
}

static bool CreateRaytracingPipeline(Renderer* renderer) {
  CD3DX12_STATE_OBJECT_DESC pipeline_desc{
      D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE};

  // DXIL Library for shaders
  {
    CD3DX12_DXIL_LIBRARY_SUBOBJECT* lib =
        pipeline_desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    D3D12_SHADER_BYTECODE libdxil =
        CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, sizeof(g_pRaytracing));
    lib->SetDXILLibrary(&libdxil);
    lib->DefineExport(L"CameraRaygenShader");
    lib->DefineExport(L"CameraClosestHitShader");
    lib->DefineExport(L"CameraMissShader");
    lib->DefineExport(L"ShadowMissShader");
  }

  // Hit group: collection of shaders describing types of hits
  // Types include closest hit, any hit, intersection shaders

  // Camera hit group
  {
    CD3DX12_HIT_GROUP_SUBOBJECT* hit_group =
        pipeline_desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    hit_group->SetClosestHitShaderImport(L"CameraClosestHitShader");
    hit_group->SetHitGroupExport(L"CameraHitGroup");
    hit_group->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
  }

  // Shader config
  {
    auto shader_config =
        pipeline_desc
            .CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    u32 payload_size = 4 * sizeof(float);
    u32 attribute_size = 2 * sizeof(float);
    shader_config->Config(payload_size, attribute_size);
  }

  // Raygen Local Root Signature
  {
    auto local_root =
        pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    local_root->SetRootSignature(renderer->raygen_local_signature);
    auto local_assoc = pipeline_desc.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    local_assoc->SetSubobjectToAssociate(*local_root);
    local_assoc->AddExport(L"CameraRaygenShader");
  }

  // Hitgroup Local Root Signature
  {
    auto local_root =
        pipeline_desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
    local_root->SetRootSignature(renderer->hitgroup_local_signature);
    auto local_assoc = pipeline_desc.CreateSubobject<
        CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
    local_assoc->SetSubobjectToAssociate(*local_root);
    local_assoc->AddExport(L"CameraHitGroup");
  }

  // Global Root Signature
  {
    auto global_root =
        pipeline_desc
            .CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    global_root->SetRootSignature(renderer->global_signature);
  }

  // Pipeline Config - Set maximum recursion depth
  {
    auto pipeline_config =
        pipeline_desc
            .CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipeline_config->Config(2);
  }

  HRESULT hr = renderer->device->CreateStateObject(
      pipeline_desc, IID_PPV_ARGS(&renderer->rt_pipeline));
  DXCHECKM(hr, "Failed to create raytracing pipeline!");
  return false;
}

// TODO: Replace this with correct memory management
// Temporary utility to quickly upload a struct to host visible memory
static bool CreateUploadBuffer(Renderer* renderer, void* data, s64 data_size,
                               ID3D12Resource** resource) {
  auto upload_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);
  HRESULT hr = renderer->device->CreateCommittedResource(
      &upload_heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(resource));
  DXCHECKM(hr, "Failed to create upload buffer!");
  void* mapped_data = nullptr;
  (*resource)->Map(0, nullptr, &mapped_data);
  CopyMemory(mapped_data, data, data_size);
  (*resource)->Unmap(0, nullptr);
  return false;
}

// TODO: Replace this with correct memory management
// Temporary utility to quickly create an upload buffer
static bool CreateUploadBuffer(Renderer* renderer, s64 data_size,
                               ID3D12Resource** resource) {
  auto upload_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(data_size);
  HRESULT hr = renderer->device->CreateCommittedResource(
      &upload_heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(resource));
  DXCHECKM(hr, "Failed to create upload buffer!");
  return false;
}

// TODO: Replace this with correct memory management
// Temporary utility to quickly create a UAV Buffer in given state
static bool CreateUavBuffer(Renderer* renderer, s64 data_size,
                            D3D12_RESOURCE_STATES initial_state,
                            ID3D12Resource** resource) {
  auto default_heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(
      data_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  HRESULT hr = renderer->device->CreateCommittedResource(
      &default_heap_props, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state,
      nullptr, IID_PPV_ARGS(resource));
  DXCHECKM(hr, "Failed to create UAV buffer!");
  return false;
}

// TODO: Replace this with Device-Local Buffers
// Create vertex, index and material buffers for the scene
static bool CreateGeometry(Application* app) {
  Renderer* renderer = app->renderer;
  Scene* scene = app->scene;
  bool failed =
      CreateUploadBuffer(renderer, scene->resources->vertices,
                         scene->resources->vertex_count * sizeof(Vertex),
                         &renderer->vertex_buffer);
  failed |= CreateUploadBuffer(renderer, scene->resources->indices,
                               scene->resources->index_count * sizeof(Index),
                               &renderer->index_buffer);
  failed |=
      CreateUploadBuffer(renderer, scene->resources->materials,
                         scene->resources->material_count * sizeof(Material),
                         &renderer->material_buffer);

  // TODO: Wrap this in a utility function?
  UINT heap_size = renderer->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
      renderer->descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
      renderer->frame_count, heap_size);

  // SRV for Vertex Buffer
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
  srv_desc.Buffer.FirstElement = 0;
  srv_desc.Buffer.NumElements = scene->resources->vertex_count;
  srv_desc.Buffer.StructureByteStride = sizeof(Vertex);
  srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
  srv_desc.Format = DXGI_FORMAT_UNKNOWN;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  renderer->device->CreateShaderResourceView(renderer->vertex_buffer, &srv_desc,
                                             cpu_handle);
  cpu_handle.Offset(1, heap_size);

  // SRV for Index Buffer
  srv_desc.Buffer.NumElements = scene->resources->index_count;
  srv_desc.Buffer.StructureByteStride = 0;
  srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
  srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
  renderer->device->CreateShaderResourceView(renderer->index_buffer, &srv_desc,
                                             cpu_handle);
  cpu_handle.Offset(1, heap_size);

  // SRV for Material Buffer
  srv_desc.Buffer.NumElements = scene->resources->material_count;
  srv_desc.Buffer.StructureByteStride = sizeof(Material);
  srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
  srv_desc.Format = DXGI_FORMAT_UNKNOWN;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  renderer->device->CreateShaderResourceView(renderer->material_buffer,
                                             &srv_desc, cpu_handle);
  return failed;
}

static bool CreateHitgroupResources(Application* app) {
  bool failed = false;
  Renderer* renderer = app->renderer;
  UINT desc_size = renderer->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
      renderer->descriptor_heap->GetCPUDescriptorHandleForHeapStart(),
      renderer->frame_count + 3, desc_size);
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    failed |=
        CreateUploadBuffer(renderer, sizeof(HitGroupConstant),
                           &renderer->rt_hitgroup_constant_buffer[frame_i]);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc{};
    cbv_desc.BufferLocation =
        renderer->rt_hitgroup_constant_buffer[frame_i]->GetGPUVirtualAddress();
    cbv_desc.SizeInBytes = sizeof(HitGroupConstant);
    renderer->device->CreateConstantBufferView(&cbv_desc, cpu_handle);
    cpu_handle.Offset(1, desc_size);

    failed |= CreateUploadBuffer(renderer,
                                 sizeof(Instance) * (app->scene->max_entities),
                                 &renderer->instance_buffer[frame_i]);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srv_desc.Buffer.NumElements = app->scene->max_entities;
    srv_desc.Buffer.StructureByteStride = sizeof(Instance);
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    renderer->device->CreateShaderResourceView(
        renderer->instance_buffer[frame_i], &srv_desc, cpu_handle);
    cpu_handle.Offset(1, desc_size);
  }

  return failed;
}

struct RendererJobParams {
  Application* app;
  u32 frame_i;
  u32 thread_i;
};

static bool CreateAccelerationStructures(RendererJobParams* job_params) {
  u32 frame_i = job_params->frame_i;
  u32 thread_i = job_params->thread_i;
  Application* app = job_params->app;
  Renderer* renderer = job_params->app->renderer;
  ID3D12GraphicsCommandList6* cmd = renderer->command_lists[frame_i][thread_i];

  // Calculate size and scratch size requirements for maximum possible TLAS
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs{};
  top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  top_level_inputs.Flags = build_flags;
  top_level_inputs.NumDescs = app->scene->max_entities;
  top_level_inputs.Type =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
  top_level_prebuild_info{};
  renderer->device->GetRaytracingAccelerationStructurePrebuildInfo(
      &top_level_inputs, &top_level_prebuild_info);
  ASSERT(top_level_prebuild_info.ResultDataMaxSizeInBytes > 0,
         "Failed to get TLAS prebuild info!");
  s64 scratch_size = top_level_prebuild_info.ScratchDataSizeInBytes;

  // Populate BLAS creation structs for each mesh
  // This is a geometry description + BLAS input
  for (u32 mesh_i = 0; mesh_i < app->scene->resources->mesh_count; mesh_i++) {
    Mesh mesh = app->scene->resources->meshes[mesh_i];

    D3D12_RAYTRACING_GEOMETRY_DESC& geometry_desc =
        renderer->rt_geometries[mesh_i];
    geometry_desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC tri_desc{};
    tri_desc.IndexBuffer = renderer->index_buffer->GetGPUVirtualAddress() +
                           mesh.index_offset * sizeof(Index);
    tri_desc.IndexCount = mesh.index_count;
    tri_desc.IndexFormat = renderer->format_library.index;
    tri_desc.Transform3x4 = 0;
    tri_desc.VertexBuffer.StartAddress =
        renderer->vertex_buffer->GetGPUVirtualAddress() +
        mesh.vertex_offset * sizeof(Vertex);
    tri_desc.VertexBuffer.StrideInBytes = sizeof(Vertex);
    tri_desc.VertexCount = mesh.vertex_count;
    tri_desc.VertexFormat = renderer->format_library.vertex_position;
    geometry_desc.Triangles = tri_desc;
    geometry_desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottom_level_inputs =
        renderer->rt_blas_inputs[mesh_i];
    bottom_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottom_level_inputs.Flags = build_flags;
    bottom_level_inputs.NumDescs = 1;
    bottom_level_inputs.Type =
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottom_level_inputs.pGeometryDescs = &renderer->rt_geometries[mesh_i];
  }
  // Acceleration Structure Creation Involves 3 GPU resources
  // 1. Scratch Space - UAV Buffer used only to create AS, can be freed after
  // 2. AS Buffer - UAV Buffer used to store the AS, must be preserved
  // 3. Instance Buffer - Regular Buffer used to build TLAS
  bool failed = false;

  // Create BLAS UAVs - one per mesh
  // Also calculate maximum scratch space needed
  for (u32 mesh_i = 0; mesh_i < app->scene->resources->mesh_count; mesh_i++) {
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO
    bottom_level_prebuild_info{};
    renderer->device->GetRaytracingAccelerationStructurePrebuildInfo(
        &renderer->rt_blas_inputs[mesh_i], &bottom_level_prebuild_info);
    ASSERT(bottom_level_prebuild_info.ResultDataMaxSizeInBytes > 0,
           "Failed to get BLAS prebuild info!");
    // Get max scratch size
    s64 blas_scratch_size = bottom_level_prebuild_info.ScratchDataSizeInBytes;
    scratch_size = max(scratch_size, blas_scratch_size);
    // Create BLAS UAV
    failed |= CreateUavBuffer(
        renderer, bottom_level_prebuild_info.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        &renderer->rt_blas[mesh_i]);
  }
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    // Create scratch UAV
    failed = CreateUavBuffer(renderer, scratch_size,
                             D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                             &renderer->as_scratch_buffer[frame_i]);
    // Create TLAS UAV
    failed |= CreateUavBuffer(
        renderer, top_level_prebuild_info.ResultDataMaxSizeInBytes,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        &renderer->rt_tlas[frame_i]);
  }
  // Create Instance Upload Buffer
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    CreateUploadBuffer(
        renderer,
        app->scene->max_entities * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
        &renderer->as_instance_buffer[frame_i]);
  }

  // Create BLAS - one per mesh
  for (u32 mesh_i = 0; mesh_i < app->scene->resources->mesh_count; mesh_i++) {
    // BLAS description
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottom_level_desc{};
    bottom_level_desc.DestAccelerationStructureData =
        renderer->rt_blas[mesh_i]->GetGPUVirtualAddress();
    bottom_level_desc.Inputs = renderer->rt_blas_inputs[mesh_i];
    bottom_level_desc.ScratchAccelerationStructureData =
        renderer->as_scratch_buffer[0]->GetGPUVirtualAddress();

    // Actually create the AS
    cmd->BuildRaytracingAccelerationStructure(&bottom_level_desc, 0, nullptr);
    auto blas_barrier =
        CD3DX12_RESOURCE_BARRIER::UAV(renderer->rt_blas[mesh_i]);
    cmd->ResourceBarrier(1, &blas_barrier);
  }

  return false;
}

// Wait for all fences at frame_i
static bool WaitForFences(Renderer* renderer, u32 frame_i) {
  for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
    if (renderer->fences[frame_i][thread_i]->GetCompletedValue() !=
        renderer->fence_values[frame_i][thread_i]) {
      renderer->fences[frame_i][thread_i]->SetEventOnCompletion(
          renderer->fence_values[frame_i][thread_i],
          renderer->fence_events[frame_i][thread_i]);
      WaitForSingleObject(renderer->fence_events[frame_i][thread_i], INFINITE);
    }
  }
  return false;
}

// Wait for frame resources to be available, then prepare command lists
// Find current backbuffer, and set out_frame_i to the same
static bool BeginFrame(Renderer* renderer, u32* out_frame_i) {
  u32 frame_i = renderer->swapchain->GetCurrentBackBufferIndex();
  WaitForFences(renderer, frame_i);
  for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
    HRESULT hr = renderer->command_allocs[frame_i][thread_i]->Reset();
    DXCHECKM(hr, "Failed to reset command allocator!");
    hr = renderer->command_lists[frame_i][thread_i]->Reset(
        renderer->command_allocs[frame_i][thread_i], nullptr);
    DXCHECKM(hr, "Failed to reset command list!");
  }
  *out_frame_i = frame_i;
  return false;
}

// Submit command lists to queue, signal fences
static bool EndFrame(Renderer* renderer, u32 frame_i, b32 present) {
  for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
    HRESULT hr = renderer->command_lists[frame_i][thread_i]->Close();
    DXCHECKM(hr, "Failed to close command list!");
  }
  renderer->command_queue->ExecuteCommandLists(
      renderer->thread_count,
      (ID3D12CommandList**)renderer->command_lists[frame_i]);
  if (present) {
    DXGI_PRESENT_PARAMETERS present_params{};
    present_params.DirtyRectsCount = 0;
    present_params.pDirtyRects = nullptr;
    present_params.pScrollRect = nullptr;
    present_params.pScrollOffset = nullptr;
    HRESULT hr = renderer->swapchain->Present1(1, 0, &present_params);
    DXCHECKM(hr, "Failed to present!");
  }
  for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
    renderer->fence_values[frame_i][thread_i]++;
    renderer->command_queue->Signal(renderer->fences[frame_i][thread_i],
                                    renderer->fence_values[frame_i][thread_i]);
  }
  return false;
}

// Create Shader Table Resources
static bool CreateShaderTableResources(RendererJobParams* job_params) {
  Renderer* renderer = job_params->app->renderer;
  u32 frame_i = job_params->frame_i;
  u32 thread_i = job_params->thread_i;
  ID3D12GraphicsCommandList6* cmd = renderer->command_lists[frame_i][thread_i];
  bool failed = false;

  ID3D12StateObjectProperties* pipeline_props = nullptr;
  renderer->rt_pipeline->QueryInterface(IID_PPV_ARGS(&pipeline_props));
  void* raygen_shader_id =
      pipeline_props->GetShaderIdentifier(L"CameraRaygenShader");
  void* camera_miss_shader_id =
      pipeline_props->GetShaderIdentifier(L"CameraMissShader");
  void* camera_hitgroup_shader_id =
      pipeline_props->GetShaderIdentifier(L"CameraHitGroup");
  void* shadow_miss_shader_id =
      pipeline_props->GetShaderIdentifier(L"ShadowMissShader");
  UINT shader_id_size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
  pipeline_props->Release();

  // Raygen shader table - one for each frame
  {
    for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
      failed |=
          CreateUploadBuffer(renderer, sizeof(RaygenConstant),
                             &renderer->rt_raygen_constant_buffer[frame_i]);
      D3D12_GPU_VIRTUAL_ADDRESS raygen_cb_address =
          renderer->rt_raygen_constant_buffer[frame_i]->GetGPUVirtualAddress();
      UINT shader_record_size =
          shader_id_size + sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
      failed |= CreateUploadBuffer(renderer, shader_record_size,
                                   &renderer->rt_raygen_shader_table[frame_i]);
      void* shader_table_data = nullptr;
      renderer->rt_raygen_shader_table[frame_i]->Map(0, nullptr,
                                                     &shader_table_data);
      memcpy(shader_table_data, raygen_shader_id, shader_id_size);
      memcpy((char*)shader_table_data + shader_id_size, &raygen_cb_address,
             sizeof(D3D12_GPU_VIRTUAL_ADDRESS));
      renderer->rt_raygen_shader_table[frame_i]->Unmap(0, nullptr);
    }
  }

  // Miss shader table - single miss shader for all frames
  {
    UINT shader_record_size = shader_id_size * 2;
    failed |= CreateUploadBuffer(renderer, shader_record_size,
                                 &renderer->rt_miss_shader_table);
    void* shader_table_data = nullptr;
    renderer->rt_miss_shader_table->Map(0, nullptr, &shader_table_data);
    memcpy(shader_table_data, camera_miss_shader_id, shader_id_size);
    memcpy((char*)shader_table_data + shader_id_size, shadow_miss_shader_id,
           shader_id_size);
    renderer->rt_miss_shader_table->Unmap(0, nullptr);
  }

  // Hit group shader table - one for each frame
  {
    for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
      auto hitgroup_dt_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
          renderer->descriptor_heap->GetGPUDescriptorHandleForHeapStart(),
          renderer->frame_count + 3 + frame_i * 2,
          renderer->device->GetDescriptorHandleIncrementSize(
              D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
      UINT shader_record_stride = shader_id_size + sizeof(hitgroup_dt_handle);
      // Hitgroup shader records must have stride multiple of 32
      // Hitgroup shader records must have size multiple of stride (32)
      shader_record_stride = 32 * ((shader_record_stride + 31) / 32);
      UINT shader_record_size = shader_record_stride;
      failed |=
          CreateUploadBuffer(renderer, shader_record_size,
                             &renderer->rt_hitgroup_shader_table[frame_i]);
      void* shader_table_data = nullptr;
      renderer->rt_hitgroup_shader_table[frame_i]->Map(0, nullptr,
                                                       &shader_table_data);
      memcpy(shader_table_data, camera_hitgroup_shader_id, shader_id_size);
      memcpy((char*)shader_table_data + shader_id_size, &hitgroup_dt_handle,
             sizeof(hitgroup_dt_handle));
      renderer->rt_hitgroup_shader_table[frame_i]->Unmap(0, nullptr);
    }
  }

  return false;
}

bool CreateRenderer(RendererCreateInfo* renderer_ci, Application* app) {
  // Allocate data
  // TODO: Move this to its own function?
  app->renderer =
      (Renderer*)StackAllocate(app->alloc, sizeof(Renderer), alignof(Renderer));
  u32 mesh_count = app->scene->resources->mesh_count;
  app->renderer->rt_blas = SALLOC(app->alloc, ID3D12Resource*, mesh_count);
  app->renderer->rt_geometries =
      SALLOC(app->alloc, D3D12_RAYTRACING_GEOMETRY_DESC, mesh_count);
  app->renderer->rt_blas_inputs = SALLOC(
      app->alloc, D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS, 1);
  for (u32 frame_i = 0; frame_i < renderer_ci->frame_count; frame_i++) {
    app->renderer->rt_instances[frame_i] = SALLOC(
        app->alloc, D3D12_RAYTRACING_INSTANCE_DESC, (app->scene->max_entities));
    app->renderer->instances[frame_i] =
        SALLOC(app->alloc, Instance, (app->scene->max_entities));
  }

  Renderer* renderer = app->renderer;
  // Fill renderer details
  renderer->width = renderer_ci->width;
  renderer->height = renderer_ci->height;
  renderer->frame_count = renderer_ci->frame_count;
  renderer->thread_count = renderer_ci->thread_count;

  ID3D12Debug* debug_interface;
  HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface));
  DXCHECK(hr);
  debug_interface->EnableDebugLayer();
  debug_interface->Release();

  IDXGIInfoQueue* info_queue;
  DXGIGetDebugInterface1(0, IID_PPV_ARGS(&info_queue));
  info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL,
                                 DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
  info_queue->SetBreakOnSeverity(
      DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
  info_queue->Release();

  // Create DXGI Factory
  hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
                          IID_PPV_ARGS(&(renderer->factory)));
  DXCHECK(hr);

  // Create Logical Device
  IDXGIAdapter1* gpu_adapter;
  GetGpuAdapter(renderer->factory, &gpu_adapter);
  hr = D3D12CreateDevice(gpu_adapter, D3D_FEATURE_LEVEL_11_0,
                         IID_PPV_ARGS(&(renderer->device)));
  DXCHECK(hr);
  gpu_adapter->Release();

  // Gather formats
  if (CreateFormatLibrary(renderer)) return true;

  // Thread Pool to create resources
  JobQueue* queue = app->threadpool->queue;
  Job* jobs =
      (Job*)StackAllocateArray(app->alloc, 10, sizeof(Job), alignof(Job));
  jobs[0] = {(job_func)CreateCommandQueue, renderer};
  jobs[1] = {(job_func)CreateCommandAllocators, renderer};
  jobs[2] = {(job_func)CreateFences, renderer};
  jobs[3] = {(job_func)CreateRootSignatures, renderer};
  jobs[4] = {(job_func)CreateDescriptorHeaps, renderer};
  PushJobs(queue, jobs, 5);
  WaitThreadQueue(queue);
  jobs[0] = {(job_func)CreateSwapchain, app};
  jobs[1] = {(job_func)CreateCommandLists, renderer};
  jobs[2] = {(job_func)CreateRaytracingOutputBuffers, renderer};
  jobs[3] = {(job_func)CreateRaytracingPipeline, renderer};
  PushJobs(queue, jobs, 4);
  WaitThreadQueue(queue);
  jobs[0] = {(job_func)CreateGeometry, app};
  PushJobs(queue, jobs, 1);
  WaitThreadQueue(queue);
  jobs[0] = {(job_func)CreateHitgroupResources, app};
  PushJobs(queue, jobs, 1);
  WaitThreadQueue(queue);

  // Begin a non-present frame for this
  u32 frame_i = 0;
  BeginFrame(renderer, &frame_i);

  // These operations require Command Lists
  // Examples: Transfer to GPU, Build Acceleration Structures, etc.
  // Allocate Params for them
  RendererJobParams* job_params = (RendererJobParams*)StackAllocateArray(
      app->alloc, renderer->thread_count, sizeof(RendererJobParams),
      alignof(RendererJobParams));
  for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++)
    job_params[thread_i] = {app, frame_i, thread_i};

  u32 num_jobs = 2;
  jobs[0].callback = (job_func)CreateAccelerationStructures;
  jobs[1].callback = (job_func)CreateShaderTableResources;

  for (u32 job_i = 0; job_i < num_jobs; job_i += renderer->thread_count) {
    u32 batch_size = (job_i + renderer->thread_count <= num_jobs)
                         ? (renderer->thread_count)
                         : (num_jobs - job_i);
    for (u32 thread_i = 0; thread_i < batch_size; thread_i++)
      jobs[job_i + thread_i].data = job_params + thread_i;
    PushJobs(queue, jobs + job_i, batch_size);
    WaitThreadQueue(queue);
  }
  EndFrame(renderer, frame_i, false);

  StackFree(app->alloc);
  return false;
}

bool BuildTlas(RendererJobParams* job_params) {
  Application* app = job_params->app;
  Renderer* renderer = app->renderer;
  u32 frame_i = job_params->frame_i;
  u32 thread_i = job_params->thread_i;
  ID3D12GraphicsCommandList6* cmd = renderer->command_lists[frame_i][thread_i];

  // Traverse scene hierarchy
  for (u32 entity_i = 0; entity_i < app->scene->entity_count; entity_i++) {
    u32 mesh_i = app->scene->entities[entity_i];
    Mesh mesh = app->scene->resources->meshes[mesh_i];

    renderer->rt_instances[frame_i][entity_i].AccelerationStructure =
        renderer->rt_blas[mesh_i]->GetGPUVirtualAddress();
    renderer->rt_instances[frame_i][entity_i]
        .InstanceContributionToHitGroupIndex = 0;
    renderer->rt_instances[frame_i][entity_i].InstanceID = entity_i;
    renderer->rt_instances[frame_i][entity_i].InstanceMask = 1;
    {
      // Store matrix transpose in row-major order
      r32 mat[16];
      MStore(app->scene->transforms[entity_i], mat);
      for (u32 row_i = 0; row_i < 3; row_i++) {
        for (u32 col_i = 0; col_i < 4; col_i++) {
          renderer->rt_instances[frame_i][entity_i].Transform[row_i][col_i] =
              mat[row_i + col_i * 4];
        }
      }
    }

    // Fill instance buffer
    renderer->instances[frame_i][entity_i] = {
        (i32)mesh.vertex_offset, (i32)mesh.index_offset,
        (i32)app->scene->material_ids[entity_i]};
  }

  // Write to resource
  void* instance_data;
  renderer->as_instance_buffer[frame_i]->Map(0, nullptr, &instance_data);
  memcpy(instance_data, renderer->rt_instances[frame_i],
         app->scene->entity_count * sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
  renderer->as_instance_buffer[frame_i]->Unmap(0, nullptr);

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS build_flags =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS top_level_inputs{};
  top_level_inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  top_level_inputs.Flags = build_flags;
  top_level_inputs.NumDescs = app->scene->entity_count;
  top_level_inputs.Type =
      D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  top_level_inputs.InstanceDescs =
      renderer->as_instance_buffer[frame_i]->GetGPUVirtualAddress();

  // TLAS description
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC top_level_desc{};
  top_level_desc.DestAccelerationStructureData =
      renderer->rt_tlas[frame_i]->GetGPUVirtualAddress();
  top_level_desc.Inputs = top_level_inputs;
  top_level_desc.ScratchAccelerationStructureData =
      renderer->as_scratch_buffer[frame_i]->GetGPUVirtualAddress();

  cmd->BuildRaytracingAccelerationStructure(&top_level_desc, 0, nullptr);
  CD3DX12_RESOURCE_BARRIER tlas_barrier =
      CD3DX12_RESOURCE_BARRIER::UAV(renderer->rt_tlas[frame_i]);
  cmd->ResourceBarrier(1, &tlas_barrier);
  return false;
}

void UpdateRenderer(Application* app) {
  Renderer* renderer = app->renderer;
  u32 frame_i = 0;
  BeginFrame(renderer, &frame_i);
  ID3D12GraphicsCommandList6* cmd = renderer->command_lists[frame_i][0];

  // Copy raygen constant buffer
  RaygenConstant raygen = {{-1.0f, 1.0f, 1.0f, -1.0f},
                           *(app->scene->main_camera)};
  void* raygen_data = nullptr;
  renderer->rt_raygen_constant_buffer[frame_i]->Map(0, nullptr, &raygen_data);
  memcpy(raygen_data, &raygen, sizeof(raygen));
  renderer->rt_raygen_constant_buffer[frame_i]->Unmap(0, nullptr);

  // Copy hitgroup constant buffer
  void* hitgroup_data = nullptr;
  renderer->rt_hitgroup_constant_buffer[frame_i]->Map(0, nullptr,
                                                      &hitgroup_data);
  i32 num_lights = min(kMaxPointLights, app->scene->light_count);
  memcpy((char*)hitgroup_data, app->scene->lights,
         num_lights * sizeof(PointLight));
  memcpy((char*)hitgroup_data + offsetof(HitGroupConstant, point_light_count),
         &num_lights, sizeof(num_lights));
  renderer->rt_hitgroup_constant_buffer[frame_i]->Unmap(0, nullptr);

  // Build TLAS from scene data
  RendererJobParams job_params = {app, frame_i, 0};
  BuildTlas(&job_params);

  // Copy instance buffer
  void* instance_data = nullptr;
  renderer->instance_buffer[frame_i]->Map(0, nullptr, &instance_data);
  i32 num_instances = app->scene->entity_count;
  memcpy(instance_data, renderer->instances[frame_i],
         sizeof(Instance) * num_instances);
  renderer->instance_buffer[frame_i]->Unmap(0, nullptr);

  // Setup Raytracing
  cmd->SetComputeRootSignature(renderer->global_signature);
  cmd->SetDescriptorHeaps(1, &renderer->descriptor_heap);
  cmd->SetComputeRootDescriptorTable(
      0, CD3DX12_GPU_DESCRIPTOR_HANDLE(
             renderer->descriptor_heap->GetGPUDescriptorHandleForHeapStart(),
             frame_i,
             renderer->device->GetDescriptorHandleIncrementSize(
                 D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
  cmd->SetComputeRootDescriptorTable(
      2, CD3DX12_GPU_DESCRIPTOR_HANDLE(
             renderer->descriptor_heap->GetGPUDescriptorHandleForHeapStart(),
             renderer->frame_count,
             renderer->device->GetDescriptorHandleIncrementSize(
                 D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
  cmd->SetComputeRootShaderResourceView(
      1, renderer->rt_tlas[frame_i]->GetGPUVirtualAddress());

  // Perform Raytracing
  D3D12_DISPATCH_RAYS_DESC dispatch_desc{};
  dispatch_desc.HitGroupTable.StartAddress =
      renderer->rt_hitgroup_shader_table[frame_i]->GetGPUVirtualAddress();
  dispatch_desc.HitGroupTable.SizeInBytes =
      renderer->rt_hitgroup_shader_table[frame_i]->GetDesc().Width;
  dispatch_desc.HitGroupTable.StrideInBytes =
      dispatch_desc.HitGroupTable.SizeInBytes;
  dispatch_desc.MissShaderTable.StartAddress =
      renderer->rt_miss_shader_table->GetGPUVirtualAddress();
  dispatch_desc.MissShaderTable.SizeInBytes =
      renderer->rt_miss_shader_table->GetDesc().Width;
  dispatch_desc.MissShaderTable.StrideInBytes =
      dispatch_desc.MissShaderTable.SizeInBytes / 2;
  dispatch_desc.RayGenerationShaderRecord.StartAddress =
      renderer->rt_raygen_shader_table[frame_i]->GetGPUVirtualAddress();
  dispatch_desc.RayGenerationShaderRecord.SizeInBytes =
      renderer->rt_raygen_shader_table[frame_i]->GetDesc().Width;
  dispatch_desc.Width = renderer->width;
  dispatch_desc.Height = renderer->height;
  dispatch_desc.Depth = 1;
  cmd->SetPipelineState1(renderer->rt_pipeline);
  cmd->DispatchRays(&dispatch_desc);

  // Transition before transfer
  {
    CD3DX12_RESOURCE_BARRIER swapchain_trans =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderer->swapchain_buffers[frame_i], D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_COPY_DEST);
    CD3DX12_RESOURCE_BARRIER uav_trans = CD3DX12_RESOURCE_BARRIER::Transition(
        renderer->rt_output_buffers[frame_i],
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    CD3DX12_RESOURCE_BARRIER trans[] = {swapchain_trans, uav_trans};
    cmd->ResourceBarrier(2, trans);
  }

  // Transfer UAV Raytracing output to Swapchain Image
  cmd->CopyResource(renderer->swapchain_buffers[frame_i],
                    renderer->rt_output_buffers[frame_i]);

  // Transition after transfer
  {
    CD3DX12_RESOURCE_BARRIER swapchain_trans =
        CD3DX12_RESOURCE_BARRIER::Transition(
            renderer->swapchain_buffers[frame_i],
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
    CD3DX12_RESOURCE_BARRIER uav_trans = CD3DX12_RESOURCE_BARRIER::Transition(
        renderer->rt_output_buffers[frame_i], D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    CD3DX12_RESOURCE_BARRIER trans[] = {swapchain_trans, uav_trans};
    cmd->ResourceBarrier(2, trans);
  }
  EndFrame(renderer, frame_i, true);
}

void DestroyRenderer(Application* app) {
  Renderer* renderer = app->renderer;

  if (renderer == nullptr) return;
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    WaitForFences(renderer, frame_i);
    for (u32 thread_i = 0; thread_i < renderer->thread_count; thread_i++) {
      DXRELEASE(renderer->command_allocs[frame_i][thread_i]);
      DXRELEASE(renderer->command_lists[frame_i][thread_i]);
      DXRELEASE(renderer->fences[frame_i][thread_i]);
      CloseHandle(renderer->fence_events[frame_i][thread_i]);
    }
    DXRELEASE(renderer->swapchain_buffers[frame_i]);
    DXRELEASE(renderer->rt_output_buffers[frame_i]);
  }

  DXRELEASE(renderer->vertex_buffer);
  DXRELEASE(renderer->index_buffer);
  DXRELEASE(renderer->material_buffer);

  DXRELEASE(renderer->rt_pipeline);
  DXRELEASE(renderer->rt_miss_shader_table);
  for (u32 frame_i = 0; frame_i < renderer->frame_count; frame_i++) {
    // Temporary, TODO: remove these parameters later
    DXRELEASE(renderer->as_scratch_buffer[frame_i]);
    DXRELEASE(renderer->as_instance_buffer[frame_i]);
    DXRELEASE(renderer->rt_tlas[frame_i]);
    DXRELEASE(renderer->rt_raygen_shader_table[frame_i]);
    DXRELEASE(renderer->rt_raygen_constant_buffer[frame_i]);
    DXRELEASE(renderer->rt_hitgroup_shader_table[frame_i]);
    DXRELEASE(renderer->rt_hitgroup_constant_buffer[frame_i]);
    DXRELEASE(renderer->instance_buffer[frame_i]);
  }

  for (u32 mesh_i = 0; mesh_i < app->scene->resources->mesh_count; mesh_i++) {
    DXRELEASE(renderer->rt_blas[mesh_i]);
  }

  DXRELEASE(renderer->command_queue);
  DXRELEASE(renderer->global_signature);
  DXRELEASE(renderer->raygen_local_signature);
  DXRELEASE(renderer->hitgroup_local_signature);
  DXRELEASE(renderer->rtv_descriptor_heap);
  DXRELEASE(renderer->descriptor_heap);
  DXRELEASE(renderer->swapchain);
  DXRELEASE(renderer->device);
  DXRELEASE(renderer->factory);

#ifdef RENDER_DEBUG
  IDXGIDebug1* dxgiDebug;
  if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
    dxgiDebug->ReportLiveObjects(
        DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY |
                                             DXGI_DEBUG_RLO_IGNORE_INTERNAL));
  }
  dxgiDebug->Release();
#endif
}
}  // namespace rally