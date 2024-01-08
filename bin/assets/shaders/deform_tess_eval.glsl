#version 430

layout(triangles, equal_spacing, ccw) in;

layout(location=0) out vec3 o_undeformedPos;

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

vec3 Deform(vec3 pos)
{
	return pos + Kelvinlet(
		pos, 
		u_localKelvinletCenter, 
		u_localKelvinletForce, 
		u_kelvinletRadius
	);
}

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
	vec3 defP = Deform(undefP);
	
	gl_Position = u_MVP * vec4(defP, 1.);
}