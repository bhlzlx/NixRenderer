#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 frag_uv;

layout ( set = 0, binding =  1 ) uniform sampler2D samBase;

layout ( location = 0 ) out vec4 outFragColor;

void main() 
{
    outFragColor = texture(samBase, frag_uv);
}