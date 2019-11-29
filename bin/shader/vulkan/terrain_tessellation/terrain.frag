#version 450

// fragment shader

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout ( location = 0 ) in vec2 texUV;
layout ( location = 1 ) in float heightLevel;

layout ( location = 0 ) out vec4 outFragColor;

layout ( set = 0, binding = 4 ) uniform sampler texSampler;
layout ( set = 0, binding = 5 ) uniform texture2D texGrass;
layout ( set = 0, binding = 6 ) uniform texture2D texSand;
layout ( set = 0, binding = 7 ) uniform texture2D texSnow;

float texLevel( float _a, float _b, float _heightLevel ) {
	return smoothstep( 0, _b - _a, -( _heightLevel - _b ));
}

void main() {

	vec4 grass = texture( sampler2D(texGrass, texSampler), texUV * 4 );
	vec4 sand = texture( sampler2D(texSand, texSampler), texUV * 4 );
	vec4 snow = texture( sampler2D(texSnow, texSampler), texUV * 4 );
	
	float grassBound = 0;
	float sandBound = 0;
	float snowBound = 0;
	
	grassBound = texLevel( 0.4, 0.5, heightLevel );
	sandBound = texLevel( 0.5, 0.75, heightLevel );
	snowBound = texLevel( 1.0, 2.0, heightLevel );
	
	sandBound -= grassBound ;// * step( 0.1f, grassBound );
	
	float grassSandBound = sandBound + grassBound;
	
	snowBound -= grassSandBound;
	//
	outFragColor = vec4( grass.rgb * grassBound + sand.rgb * sandBound + snow.rgb * snowBound, 1.0f );
    //outFragColor = vec4( 1.0f, 0.0f, 0.0f, 1.0f );
}