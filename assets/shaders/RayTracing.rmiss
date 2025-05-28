#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "RayPayload.glsl"
#include "UniformBufferObject.glsl"

layout(binding = 3) readonly uniform UniformBufferObjectStruct { UniformBufferObject Camera; };
layout(binding = 11) uniform samplerCube SkyboxSampler; // New binding for skybox

layout(location = 0) rayPayloadInEXT RayPayload Ray;

void main()
{
    if (Camera.HasSky)
    {
        vec3 dir = normalize(gl_WorldRayDirectionEXT);
        vec3 skyColor = texture(SkyboxSampler, dir).rgb;
        Ray.ColorAndDistance = vec4(skyColor, -1);
    }
    else
    {
        Ray.ColorAndDistance = vec4(0.0, 0.0, 0.0, -1);
    }
}
