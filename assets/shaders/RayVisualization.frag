#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

/*layout(location = 0) in vec3 FragColor;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in flat int FragMaterialIndex;*/

layout(location = 0) out vec4 OutColor;

//const vec4 dirLightColor = vec4(1.0);
//const vec3 dirLightDir = normalize(vec3(5.0, 4.0, 3.0));

void main() 
{
	//const float d = max(dot(dirLightDir, normalize(FragNormal)), 0.2);
	
	//vec3 c = FragColor * d;
	
    //OutColor = dirLightColor * vec4(c, 1);

	OutColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
}