#version 430
#include "assets/shaders/deformation.glsl"

layout(location=0) in vec3 a_position;

layout(location=0) out vec3 o_deformedPos;

void main()
{
#ifdef BONE_MODE
	FindCurrentBones(a_position);
#endif
	o_deformedPos = Deform(a_position);
	
	gl_Position = vec4(a_position, 1.);
}