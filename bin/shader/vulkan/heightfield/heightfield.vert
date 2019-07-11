#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout( set = 0, binding = 0 ) uniform Argument {
    mat4 mvp;
    uint col;
    uint row;
};

layout( set = 0, binding = 1 ) uniform sampler2D samHeightField;

layout ( location = 0 ) out float color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main(){
    float c = gl_VertexIndex % col;
    float r = gl_VertexIndex / col;
    vec2 uv = vec2( r/row,c/col );
    float height = texture( samHeightField, uv ).r;
    color = height;
    //height = height * 255.0f;
    vec4 p = vec4( c, height, r, 1.0f );
	gl_Position = mvp * p;
	gl_Position.y *= -1;
}
