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
    uint c = gl_VertexID % col;
    uint r = gl_VertexID / col;
    float height = texture( samHeightField, vec2((float)c / (float)row), (float)r / (float)col);
    color = height;
    height = height * 255.0f;
    vec3 p( c, height, r );
	gl_Position = mvp * p;
	gl_Position.y *= -1;
}
