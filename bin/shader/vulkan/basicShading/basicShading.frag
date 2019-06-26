#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2  normalMapUV;
layout (location = 1) in vec3  TBN_lightDir;
layout (location = 2) in vec3  TBN_cameraDir;

layout ( set = 0, binding = 1 ) uniform sampler2D normalMap;

layout ( location = 0 ) out vec4 outFragColor;

void main() 
{
	vec3 normal = normalize( texture( normalMap, normalMapUV ).xyz );
	//vec3 reflection = normalize( reflect( TBN_lightDir, normal.xyz ) );
	//float color=pow(max(dot(TBN_cameraDir,reflection),0.0),90.0);	
	float color = dot( normal , TBN_cameraDir.xyz);
    outFragColor = vec4( color, color, color, 1.0f );
}