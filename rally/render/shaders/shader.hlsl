#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define MAX_POINT_LIGHTS 10

struct Viewport
{
    float left;
    float top;
    float right;
    float bottom;
};

struct PerspectiveCamera
{
    float4x4 view_to_world;
    float4x4 projection_to_world;
};

struct RayGenConstantBuffer
{
    Viewport viewport;
    PerspectiveCamera camera;
};

struct Vertex
{
    float3 position;
    float _pad0;
    float3 normal;
    float _pad1;
    float3 tangent;
    float _pad2;
    float3 bitangent;
    float _pad3;
    float2 uv;
    float2 _pad4;
};

struct Material
{
    float3 albedo;
    float shininess;
};

struct PointLight
{
    float4 position;
    float3 color;
    float intensity;
};

struct PointLightSettings{
    int count;
    int _pad[47];
};

struct HitGroupConstantBuffer{
    PointLight point_lights[MAX_POINT_LIGHTS];
    PointLightSettings point_light_settings;
};

struct InstanceBuffer{
    int vertex_offset;
    int index_offset;
    int material_id;
    int _pad;
};

// Global DXR descriptors
RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

// Local Raygen Constant Buffer
ConstantBuffer<RayGenConstantBuffer> g_rayGenCB : register(b0);

// Global Descriptor Table
StructuredBuffer<Vertex> vertex_buffer: register(t1, space0);
ByteAddressBuffer index_buffer : register(t2, space0);
StructuredBuffer<Material> material_buffer : register(t3, space0);

// Local Hit Group Descriptor Table
ConstantBuffer<HitGroupConstantBuffer> hitgroup_cb : register(b1);
StructuredBuffer<InstanceBuffer> instance_buffer : register(t4, space0);

typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void MyRaygenShader()
{
    float2 lerpValues = (float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions();

    // Start with projection space coords and direction
    float3 proj_dir = float3(0, 0, 1);
    float3 proj_origin = float3(
    lerp(g_rayGenCB.viewport.left, g_rayGenCB.viewport.right, lerpValues.x),
    lerp(g_rayGenCB.viewport.top, g_rayGenCB.viewport.bottom, lerpValues.y),
    0.0f);
    // Move to world space
    float4 world_pos4 = mul(g_rayGenCB.camera.projection_to_world,float4(proj_origin,1.0f));
    float3 world_pos = world_pos4.xyz/world_pos4.w;
    // Camera position
    float4 camera_pos4 = mul(g_rayGenCB.camera.view_to_world,float4(0.0f,0.0f,0.0f,1.0f));
    float3 camera_pos = camera_pos4.xyz/camera_pos4.w;
    float3 camera_dir = normalize(world_pos-camera_pos);
    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = camera_pos;
    ray.Direction = camera_dir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

float3 InterpolateFloat3(in float3 a0, in float3 a1, in float3 a2, in float2 barycentrics){
    return a0+barycentrics.x*(a1-a0)+barycentrics.y*(a2-a0);
}

float3 GetWorldPos(){
    return WorldRayOrigin()+WorldRayDirection()*RayTCurrent();
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    uint instance_id = InstanceID();
    int vertex_offset = instance_buffer[instance_id].vertex_offset;
    int index_offset = instance_buffer[instance_id].index_offset;

    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    uint pidx = PrimitiveIndex();
    uint3 idx = index_buffer.Load3((pidx*3+index_offset)*4);
    idx += vertex_offset;
    
    // Get normal
    float3 n0 = vertex_buffer[idx.x].normal;
    float3 n1 = vertex_buffer[idx.y].normal;
    float3 n2 = vertex_buffer[idx.z].normal;
    float3 ni = InterpolateFloat3(n0,n1,n2,attr.barycentrics);
    float3 world_normal = mul(ObjectToWorld3x4(),float4(ni,0.0f));

    // Get world pos
    float3 world_pos = GetWorldPos();
    
    int mat_id = instance_buffer[instance_id].material_id;
    float3 world_camera_pos = WorldRayOrigin();
    float3 albedo_color = material_buffer[mat_id].albedo;
    float k_a = 0.1f;
    float k_d = 0.45f;
    float k_s = 0.45f;
    float shininess = material_buffer[mat_id].shininess;
    float3 N = normalize(world_normal);
    float3 V = normalize(world_camera_pos-world_pos);
    
    float3 phong = k_a*albedo_color;
    for(int light_i = 0; light_i<hitgroup_cb.point_light_settings.count; light_i++){
        PointLight point_light = hitgroup_cb.point_lights[light_i];
        float3 world_light_pos = point_light.position.xyz;
        float3 world_light_dir = world_light_pos-world_pos;
        float light_dist = length(world_light_dir);
        float light_att = clamp(point_light.intensity/light_dist,0.0f,1.0f);
        float3 L = normalize(world_light_dir);
        float3 R = normalize(reflect(-L,N));
        phong += light_att*k_d*max(dot(N,L),0.0f)*albedo_color*point_light.color;
        phong += light_att*k_s*max(pow(dot(R,V),shininess),0.0f)*albedo_color*point_light.color;
    }
    payload.color = float4(phong, 1);
}

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, 1);
}

#endif // RAYTRACING_HLSL