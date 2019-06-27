#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2  normalMapUV;
layout (location = 1) in vec3  lightDir;
layout (location = 2) in vec3  cameraDir;
layout (location = 3) in mat3x3 TBN;

layout (location = 1) in vec3 TBN_lightDir;
layout (location = 2) in vec3 TBN_cameraDir;
//layout (location = 4) in mat3x3 TBN;

layout ( set = 0, binding = 1 ) uniform sampler2D normalMap;

layout ( location = 0 ) out vec4 outFragColor;

void main() 
{
	vec3 normal =  texture( normalMap, normalMapUV ).xyz ;
    normal = normal * 2.0f - 1.0f;
	//normal = TBN * normal;
	//vec3 reflection = normalize( reflect( TBN_lightDir, normal.xyz ) );
	//float color=pow(max(dot(TBN_cameraDir,reflection),0.0),90.0);	
	float color = dot( normalize(normal) , normalize(TBN_cameraDir));
    //outFragColor = vec4(normal, 1.0f );
	outFragColor = vec4( color, color, color, 1.0f );
}