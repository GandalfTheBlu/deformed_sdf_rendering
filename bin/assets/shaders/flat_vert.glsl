#version 430

layout(location=0) in vec3 a_position;

uniform mat4 u_MVP;
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletRadius;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force, float radius) 
{
	vec3 toPoint = point - center;
    float distanceSquared = dot(toPoint, toPoint);
    float radiusSquared = radius * radius;
	
    if (distanceSquared > radiusSquared) 
	{
        return vec3(0.);
    }
	
    vec3 displacement = (
        3. * dot(toPoint, force) * (radiusSquared - distanceSquared) /
        (4. * 3.14159 * distanceSquared * distanceSquared)
    ) * toPoint;

    return displacement;
}

void main()
{
	vec3 displacedPos = a_position + Kelvinlet(
		a_position, 
		u_localKelvinletCenter, 
		u_localKelvinletForce, 
		u_kelvinletRadius
	);
	
	gl_Position = u_MVP * vec4(displacedPos, 1.);
}