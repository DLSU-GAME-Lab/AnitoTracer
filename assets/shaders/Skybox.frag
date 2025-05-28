#version 460

layout(location = 0) in vec3 texCoord;
layout(location = 0) out vec4 outColor;

const int SAMP_DIFFUSE = 0;
layout(set = 0, binding = 1) uniform samplerCube samplers[1];

void main() {
	outColor = texture(samplers[SAMP_DIFFUSE], texCoord);
}