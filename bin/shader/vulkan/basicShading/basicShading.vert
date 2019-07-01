#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;
layout (location = 1) in vec3 vert_normal;
layout (location = 2) in vec2 vert_uv;
layout (location = 3) in vec3 vert_tangent;
layout (location = 4) in vec3 vert_bitangent;

layout (location = 0) out vec2 normalMapUV;
layout (location = 1) out vec3 lightDir;
layout (location = 2) out vec3 cameraDir;
layout (location = 3) out mat3x3 TBN;

out gl_PerVertex {
    vec4 	gl_Position;
};

layout( set = 0, binding = 0 ) uniform GlobalArgument {
	mat4 projection;
	mat4 view;
	//
	vec3 sun;
	vec3 eye;
};

layout( set = 1, binding = 0 ) uniform LocalArgument {
	mat4 model;
};

void main() 
{
	vec4 worldPosition = model * vec4(vert_position, 1.0f);
	vec3 worldNormal = normalize( vec3(model * vec4( vert_normal, 0.0f)));
	vec3 worldTangent = normalize( vec3(model * vec4( vert_tangent, 0.0f)));
	vec3 worldBitan = normalize( vec3(model * vec4( vert_bitangent, 0.0f)));
	
	worldTangent = normalize(worldTangent - dot(worldTangent, worldNormal) * worldNormal);	
	worldPosition = worldPosition / worldPosition.w;
    TBN = mat3x3( worldTangent.xyz, worldBitan.xyz, worldNormal.xyz );
	// output for fragment stage
	lightDir = worldPosition.xyz - sun;
	cameraDir = eye - worldPosition.xyz;
	normalMapUV = vert_uv;
    //
	gl_Position = projection * view * worldPosition;
	gl_Position.y *= -1;
}
