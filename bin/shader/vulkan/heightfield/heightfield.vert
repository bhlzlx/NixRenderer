#version 430
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_EXT_scalar_block_layout : enable

layout( set = 0, binding = 0 ) uniform Argument {
    mat4 mvp;
    uint col;
    uint row;
	int index;
};

layout( std430, set = 0, binding = 7 ) buffer storageBuffer
{
    int some_int;
    float fixed_array[42];
    float variable_array[];
};

layout( set = 0, binding = 1 ) uniform sampler samplerHeightField; // sampler
layout( set = 0, binding = 2 ) uniform texture2D textureHeightField; // sampled texture
layout( set = 0, binding = 3 , r32f) uniform image2D storageImage; // storage image
layout( set = 0, binding = 4 ) uniform sampler2D combinedSamplerImage; // sampler & texture
//
layout( set = 0, binding = 5, r32f ) readonly uniform imageBuffer texelBuffer; // texel buffer



layout ( location = 0 ) out float color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main(){
    float c = gl_VertexIndex % col;
    float r = gl_VertexIndex / col;
    vec2 uv = vec2( r/row,c/col);
    float height = texture( sampler2D(textureHeightField,samplerHeightField), uv ).r;
    color = height;
    //height = height * 255.0f;
    vec4 p = vec4( c, height, r, 1.0f );
	gl_Position = mvp * p;
	gl_Position.y *= -1;
	//
	//gl_Position += texelFetch(tbo, index);
}
