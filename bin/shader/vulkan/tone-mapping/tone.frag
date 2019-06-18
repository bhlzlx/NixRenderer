#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 frag_uv;

layout ( set = 0, binding =  0 ) uniform sampler2D hdrImage;
layout ( set = 0, binding =  1 ) uniform ToneMappingArgument {
	float adapted_lum;
};

layout ( location = 0 ) out vec4 outFragColor;

vec3 ACESToneMapping( vec3 color, float adapted_lum )
{
	const float A = 2.51f;
	const float B = 0.03f;
	const float C = 2.43f;
	const float D = 0.59f;
	const float E = 0.14f;
	color *= adapted_lum;
	return (color * (A * color + vec3(B,B,B))) / (color * (C * color + vec3(D,D,D)) + E);
}

void main() 
{
	vec4 color = texture(hdrImage, frag_uv);
	outFragColor.rgb = ACESToneMapping( color.xyz, adapted_lum );
    outFragColor.a = 1.0f;
}