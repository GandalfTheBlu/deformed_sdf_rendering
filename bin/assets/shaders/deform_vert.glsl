#version 430

layout(location=0) in vec3 a_position;

layout(location=0) out vec3 o_deformedPos;

uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float dirCompare = dot(toPoint, force);
	
	if(dirCompare == 0.)
	{
		return vec3(0.);
	}
	
	float displacement = exp(-(dot(toPoint, toPoint) - 
		dirCompare * dirCompare / dot(force, force)));
		
	return force * displacement;
}

vec3 Deform(vec3 pos)
{
	return pos + Kelvinlet(
		pos, 
		u_localKelvinletCenter, 
		u_localKelvinletForce
	);
}

void main()
{
	o_deformedPos = Deform(a_position);
	
	gl_Position = vec4(a_position, 1.);
}