#version 450

// vertex shader output for tessellation control shader

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec2 vert_uv;

layout (location = 0) out vec3 tcs_position;
layout (location = 1) out vec2 tcs_uv;

layout( set = 0, binding = 0 ) uniform Argument1 {
	mat4 transform;
};

void main() 
{
	vec4 pos = transform * vec4(vert_position, 1.0f);
	pos.y *= -1;
	tcs_position = (pos / pos.w).xyz;
	tcs_uv = vert_uv;
}
