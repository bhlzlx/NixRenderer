#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;

out gl_PerVertex {
    vec4 gl_Position;
};

layout( set = 0, binding = 0 ) uniform Argument {
	mat4 mvp;
};

void main() 
{
	gl_Position = mvp * vec4(vert_position, 1.0f);
	gl_Position.y *= -1;
}