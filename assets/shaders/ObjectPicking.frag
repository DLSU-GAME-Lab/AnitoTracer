#version 460

layout(location = 0) in vec4 color;
precision mediump float;
layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = color;
}