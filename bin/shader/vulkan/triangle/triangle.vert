#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec2 vert_uv;

layout (location = 0) out vec3 frag_uv;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout( set = 0, binding = 0 ) uniform GlobalArgument {
	mat4 projection;
	mat4 view;
	float imageArrayIndex;
};

layout( set = 1, binding = 0 ) uniform LocalArgument {
	mat4 model;
};

void main() 
{
	gl_Position = vec4(vert_position, 1.0f);
	frag_uv = vec3(vert_uv, imageArrayIndex);
	//gl_Position = projection * view * model * vec4(vert_position, 1.0);
	gl_Position.y *= -1;
}
