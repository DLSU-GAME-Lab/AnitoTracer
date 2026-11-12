struct DirectionalLight
{
    float4 Direction; // xyz: Light direction, w: Unused padding
    float4 Color; // rgb: Color, a: Intensity
};

struct PointLight
{
    float4 Position; // xyz: Position, w: Unused padding
    float4 Color; // rgb: Color, a: Intensity
    float Range; // Max attenuation distance
    
    //Just VK things
    // Split the float3 into a float and float2 to satisfy std140 alignment rules!
    float Padding1; // 4 bytes
    float2 Padding2; // 8 bytes (Starts on an 8-byte boundary, which is legal)
    
    float4 ExtraPadding;
};

cbuffer LightConstants
{
    DirectionalLight g_DirLights[4];
    PointLight g_PointLights[8];
    int g_NumDirLights;
    int g_NumPointLights;
    float2 g_Padding;
    float4 g_CameraPos; // World space camera position
};

cbuffer ShadowSettings
{
    float g_ShadowBias;
    float g_AmbientMultiplier;
    float g_SkyLightIntensity;   // Hybrid only: strength of the sky's light (sky intensity * sky light intensity)
    float g_ReflectionIntensity; // Hybrid only: strength of sky reflections
    float4 g_AmbientColor;
    uint g_NoiseFrame;           // Changes every frame while TAA is on, so ray sample patterns vary over time; 0 keeps them still
    float2 g_SkyRotation;        // Hybrid only: (sin, cos) of the sky's rotation around the up axis, see RotateSkyDirection
    float g_ReflectionMaxRoughness; // Hybrid only: surfaces rougher than this reflect the sky without tracing a ray

    float g_ReflectionMaxDistance;  // Hybrid only: how far a reflection ray looks for geometry
    uint g_ReflectionFlags;         // Hybrid only: REFLECTION_* bits, see hybrid_shading.hlsli
    float2 g_ShadowPadding;
};