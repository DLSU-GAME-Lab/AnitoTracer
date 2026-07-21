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
    float3 Padding; // Alignment padding to 16 bytes
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