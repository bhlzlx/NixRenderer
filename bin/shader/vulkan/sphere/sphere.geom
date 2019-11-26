#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in float	geom_brightness[];
layout (location = 0) out float	frag_brightness[];

layout( triangles ) in;
layout( line_strip, max_vertices = 6 ) out;

void main() {
	vec3 normalv3 = normalize( cross(gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz) );
	vec4 normal = -vec4 (normalv3, 0.0f );
	gl_Position = gl_in[0].gl_Position; EmitVertex();
	gl_Position = gl_in[0].gl_Position + normal; EmitVertex();
	gl_Position = gl_in[1].gl_Position; EmitVertex();
	gl_Position = gl_in[1].gl_Position + normal; EmitVertex();
	gl_Position = gl_in[2].gl_Position; EmitVertex();
	gl_Position = gl_in[2].gl_Position + normal; EmitVertex();	//
	
	frag_brightness[0] = geom_brightness[0];
	frag_brightness[1] = geom_brightness[1];
	frag_brightness[2] = geom_brightness[2];
}