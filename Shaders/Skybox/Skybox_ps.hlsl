TextureCube g_Cubemap;
SamplerState g_Cubemap_sampler;

struct SkyboxPSInput
{
    float4 Pos : SV_POSITION;
    float3 SampleDir : TEXCOORD0;
};

float4 main_ps(in SkyboxPSInput In) : SV_TARGET
{
    // Sample the TextureCube using our normalized direction vector
    float3 dir = normalize(In.SampleDir);
    float4 skyColor = g_Cubemap.Sample(g_Cubemap_sampler, dir);
    
    // Optional: If you are doing HDR mapping later, you can return raw values.
    // For standard un-tonemapped LDR, just return the color!
    return skyColor;
}