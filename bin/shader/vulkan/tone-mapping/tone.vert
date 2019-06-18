#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec2 vert_uv;

layout (location = 0) out vec2 frag_uv;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	gl_Position = vec4(vert_position, 1.0f);
	gl_Position.y *= -1;
	frag_uv = vert_uv;
}
