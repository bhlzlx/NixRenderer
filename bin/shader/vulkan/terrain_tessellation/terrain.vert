#version 450

// vertex shader output for tessellation control shader

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec2 vert_uv;

layout (location = 0) out vec3 tcs_position;
layout (location = 1) out vec2 tcs_uv;

void main() 
{
	tcs_position = vert_position;
	tcs_uv = vert_uv;
}
