#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 uv;
layout (location = 2) in uint colorMask;

layout (location = 0) out vec3 frag_uv;
layout (location = 1) out vec4 frag_colorMask;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

//ayout( set = 0, binding = 0 ) uniform UIArgument {
//	float screenWidth;
//	float screenHeight;
//	float alpha;
//;

layout(push_constant) uniform UIArgument {
	float screenWidth;
	float screenHeight;
	float alpha;
};

void main() 
{
	gl_Position.x = (position.x / screenWidth) * 2 - 1.0f;
	gl_Position.y = -((position.y / screenHeight) * 2 - 1.0f);
	gl_Position.z = 0.0f;
	gl_Position.w = 1.0f;
	// -----------------------------------------------------------
	frag_uv = uv;
	frag_colorMask = vec4(colorMask>>24, colorMask>>16&0xff, colorMask>>8 &0xff, colorMask & 0xff )/255.0f;
	// return
} // end main()
