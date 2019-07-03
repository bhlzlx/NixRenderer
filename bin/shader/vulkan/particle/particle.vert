#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 vert_position;

layout( set = 0, binding = 0 ) uniform Argument {
    mat4 view;
    mat4 projection;
    //
    float wndWidth;
    float wndHeight;
    float perspectiveNear;
    float perspectiveFOV;
    float particleSize;
};

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
};

void main(){
	gl_Position = projection * view * vec4(vert_position, 1.0f);
    float sizeOfNearPlane = tan( perspectiveFOV/2 ) * perspectiveNear * 2 * min(wndHeight, wndWidth);
    gl_PointSize = sizeOfNearPlane * particleSize / gl_Position.w;
	gl_Position.y *= -1;
}
