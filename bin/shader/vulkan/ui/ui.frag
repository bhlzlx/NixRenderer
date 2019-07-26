#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 frag_uv;
layout (location = 1) in flat vec4 frag_colorMask;

layout ( set = 0, binding =  0 ) uniform sampler2DArray UiTexArray;

layout ( location = 0 ) out vec4 outFragColor;

void main()
{
    outFragColor = texture(UiTexArray, frag_uv);
	outFragColor = frag_colorMask * outFragColor;
}