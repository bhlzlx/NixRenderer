#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( set = 0, binding = 1 ) uniform sampler2D samplerParticle;

layout ( location = 0 ) out vec4 outFragColor;


void main(){

    outFragColor = texture( samplerParticle, gl_PointCoord );
    //samplerParticle();
    //outFragColor = vec4(1.0f, 0.5f, 0.5f, 1.0f);
}