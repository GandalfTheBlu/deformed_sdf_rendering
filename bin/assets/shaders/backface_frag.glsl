#version 430

layout(location=0) in vec3 i_deformedPos;

out float o_distance;

uniform vec3 u_localCameraPos;

void main()
{
	o_distance = distance(u_localCameraPos, i_deformedPos);
}