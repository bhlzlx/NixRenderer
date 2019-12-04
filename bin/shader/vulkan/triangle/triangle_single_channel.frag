#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 frag_uv;

layout ( set = 0, binding = 1 ) uniform sampler triSampler;
layout ( set = 0, binding = 2 ) uniform texture2D triTexture;

layout ( location = 0 ) out vec4 outFragColor;

void main() {
	//frag_uv = frag_uv + vec2( 0.5f, 0.5f );
    outFragColor = texture( sampler2D(triTexture, triSampler), frag_uv + vec2(0.5f, 0.5f));
	outFragColor.a = outFragColor.r;
	outFragColor.r = 1;
	outFragColor.g = 1;
	outFragColor.b = 1;
}