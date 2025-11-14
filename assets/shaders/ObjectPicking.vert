#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 vertex_position;

mat4 u_M_mvp;
uint u_modelID;
layout(location = 0) out vec4 color;

void main()
{
	gl_Position = u_M_mvp * vec4( vertex_position , 1 );

	// convert the model ID to a color
	color.r = float( ( u_modelID & uint(0x000000ff) ) >> 0 )/255.0;
	color.g = float( ( u_modelID & uint(0x0000ff00) ) >> 8 )/255.0;
	color.b = float( ( u_modelID & uint(0x00ff0000) ) >> 16 )/255.0;
	color.a = 1.0;
}
