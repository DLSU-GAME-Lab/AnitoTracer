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
    float2 g_ShadowPadding;
};