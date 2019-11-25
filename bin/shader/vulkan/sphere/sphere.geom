#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in float	frag_brightness;

out gl_PerVertex 
{
    vec4 	gl_Position;
};

layout( set = 0, binding = 0 ) uniform GlobalArgument {
	mat4 projection;
	mat4 view;
	vec3 light;
};

layout( set = 1, binding = 0 ) uniform LocalArgument {
	mat4 model;
};

void main() 
{
	vec4 worldPosition = model * vec4(vert_position, 1.0f);
	worldPosition = worldPosition / worldPosition.w;
	gl_Position = projection * view * worldPosition;
	vec4 tNormal = model * vec4( normalize(vert_normal), 0.0f );
	frag_brightness = dot( normalize(tNormal.xyz), normalize(light - worldPosition.xyz ));
	gl_Position.y *= -1;
}
