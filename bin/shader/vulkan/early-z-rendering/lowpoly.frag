#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// =============== fragment stage inputs ===============
layout (location = 0) in flat vec3  frag_normal;
layout (location = 1) in vec3 frag_position;

// =============== fragment stage output ===============
layout ( location = 0 ) out vec4 outFragColor;

// =============== fragment uniform ===============

struct Light {
    vec3 position;
    vec3 color;
};

layout( set = 0, binding = 1 ) uniform PointLight {
    float       constant;
    float       linear;
    float       quadratic;
    //
    uint        light_count;
    Light       lights[4];
};

void main() 
{
    vec3 fragColor = vec3( 0.4f, 0.4f, 0.4f );
    for( uint i = 0; i < light_count ; ++i ) {
        float distance = length(lights[i].position - frag_position);
        float attenuation = 1.0f / ( constant + linear * distance + quadratic * (distance * distance));
        fragColor = fragColor + attenuation * lights[i].color;
    }
    outFragColor = vec4( fragColor, 1.0f );
}