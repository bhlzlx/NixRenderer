#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 frag_uv;
layout (location = 1) in float frag_alpha;

layout ( set = 0, binding =  0 ) uniform sampler2DArray UiTexArray;

layout ( location = 0 ) out vec4 outFragColor;

void main()
{
    outFragColor = texture(UiTexArray, frag_uv);
	outFragColor.a = outFragColor.a * frag_alpha;
}