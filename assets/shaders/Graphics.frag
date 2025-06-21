#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "Material.glsl"

layout(binding = 1) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 2) uniform sampler2D[] TextureSamplers;

layout(location = 0) in vec4 FragColor;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in flat int FragMaterialIndex;

layout(location = 0) out vec4 OutColor;

const vec4 dirLightColor = vec4(1.0);
const vec3 dirLightDir = normalize(vec3(5.0, 4.0, 3.0));

void main() 
{
    const int textureId = Materials[FragMaterialIndex].DiffuseTextureId;
    const float d = max(dot(dirLightDir, normalize(FragNormal)), 0.2);

    vec4 baseColor = vec4(FragColor.rgb * d, FragColor.a); // Apply lighting to base color
    if (textureId >= 0)
    {
        vec4 texColor = texture(TextureSamplers[textureId], FragTexCoord);
        baseColor.rgb *= texColor.rgb;
        baseColor.a *= texColor.a; 
    }

    if (baseColor.a < 0.01)
        discard;

    OutColor = dirLightColor * baseColor;
}
