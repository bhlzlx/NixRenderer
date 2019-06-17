#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec3 vert_normal;

layout (location = 0) out vec3 frag_normal;
layout (location = 1) out vec3 frag_position;


out gl_PerVertex 
{
    vec4 	gl_Position;
};

struct Light {
    vec3 position;
    vec3 color;
};

layout( set = 0, binding = 0 ) uniform GlobalArgument {
	mat4        projection;
	mat4        view;
};

layout( set = 1, binding = 0 ) uniform LocalArgument {
	mat4 model;
};

void main() 
{
	vec4 worldPosition = model * vec4(vert_position, 1.0f);
    vec4 worldNormal = model * vec4( normalize(vert_normal), 0.0f );

	worldPosition = worldPosition / worldPosition.w;
    
    frag_position = worldPosition.xyz;
    frag_normal = worldNormal.xyz;
    //
	gl_Position = projection * view * worldPosition;
	gl_Position.y *= -1;
}
