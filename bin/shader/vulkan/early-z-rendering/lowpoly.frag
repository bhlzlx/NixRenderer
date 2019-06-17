#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// =============== fragment stage inputs ===============
layout (location = 0) in vec3 frag_normal;
layout (location = 1) in vec3 frag_position;

// =============== fragment stage output ===============
layout ( location = 0 ) out vec4 outFragColor;

// =============== fragment uniform ===============
layout( set = 0, binding = 1 ) uniform LightArgument {
    uint        light_count;
    Light       lights[4];
};

void main() 
{
    outFragColor = vec4( brightness, brightness, brightness, 1.0f );
}