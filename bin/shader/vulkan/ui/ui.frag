#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 frag_uv;
layout (location = 1) in flat vec4 frag_colorMask;

layout ( set = 0, binding =  1 ) uniform texture2DArray uiTextureArray;
layout ( set = 0, binding =  2 ) uniform sampler uiSampler;

layout ( location = 0 ) out vec4 outFragColor;

void main()
{
    outFragColor = texture( sampler2DArray(uiTextureArray,uiSampler), frag_uv);
	outFragColor = outFragColor * frag_colorMask;
}