#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 frag_color;

layout ( location = 0 ) out vec4 outFragColor;

void main() 
{
    outFragColor = vec4( frag_color, 1.0f );
}