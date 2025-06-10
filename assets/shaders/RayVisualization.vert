
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#include "UniformBufferObject.glsl"

//layout(push_constant) uniform PushConstantModelStruct { PushConstantModel Object; };

layout(binding = 0) readonly uniform UniformBufferObjectStruct { UniformBufferObject Camera; };

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec4 InColor;

layout(location = 0) flat out vec4 FragColor;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
    gl_Position = Camera.Projection * Camera.ModelView * vec4(InPosition.xyz, 1.0);
    FragColor = InColor;
}
