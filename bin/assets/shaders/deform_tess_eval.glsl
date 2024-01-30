#version 430
#include "assets/shaders/deformation.glsl"

layout(triangles, equal_spacing, ccw) in;

layout(location=0) out vec3 o_undeformedPos;

uniform mat4 u_MVP;

void main()
{
	// get undeformed position by linear interpolation
	vec3 undefP0 = gl_in[0].gl_Position.xyz;
	vec3 undefP1 = gl_in[1].gl_Position.xyz;
	vec3 undefP2 = gl_in[2].gl_Position.xyz;
	
	vec3 undefP = undefP0 * gl_TessCoord.x + undefP1 * gl_TessCoord.y + undefP2 * gl_TessCoord.z;
	
	// pass the undeformed position to the fragment shader
	o_undeformedPos = undefP;
	
	// finally, apply deformation and MVP matrix
#ifdef BONE_MODE
	FindCurrentBones(undefP);
#endif	
	vec3 defP = Deform(undefP);
	
	gl_Position = u_MVP * vec4(defP, 1.);
}