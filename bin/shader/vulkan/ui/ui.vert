#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec3 frag_uv;
layout (location = 1) out flat vec4 frag_colorMask;


struct RectParam {
	uint imageArrayIndex;
	uint colorMask;
};

layout ( set = 0, binding =  0 ) uniform RectParams {
	RectParam params[4096];
};

out gl_PerVertex 
{
    vec4 gl_Position;   
};

layout(push_constant) uniform UIArgument {
	float screenWidth;
	float screenHeight;
	uint uniformIndex;
};

void main() 
{
	gl_Position.x = (position.x / screenWidth) * 2 - 1.0f;
	gl_Position.y = (position.y / screenHeight) * 2 - 1.0f;
	gl_Position.z = 0.0f;
	gl_Position.w = 1.0f;
	// -----------------------------------------------------------
	float arrayIndex = params[uniformIndex + gl_VertexIndex/4].imageArrayIndex;
	uint colorMask = params[uniformIndex + gl_VertexIndex/4].colorMask;
	frag_uv = vec3(uv, arrayIndex);
	frag_colorMask = vec4(colorMask>>24, colorMask>>16&0xff, colorMask>>8&0xff, colorMask&0xff)/255.0f;
	// return
} // end main()
