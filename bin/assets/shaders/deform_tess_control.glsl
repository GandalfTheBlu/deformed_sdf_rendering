#version 430

layout(vertices=3) out;

layout(location=0) in vec3 i_deformedPos[];

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

float LinearizationError(vec3 undeformedPos, vec3 deformedPos)
{
	// differance between the interpolated deformed position and the deformed interpolated position
	return distance(deformedPos, Deform(undeformedPos));
}

void main()
{
	// pass the undeformed vertex to the tesselation evaluation shader
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	
	// only one vertex per patch handles tesselation
	if(gl_InvocationID != 0)
	{
		return;
	}
	
	// calculate the linearization error across the triangle
	vec3 undefP0 = gl_in[0].gl_Position.xyz;
	vec3 undefP1 = gl_in[1].gl_Position.xyz;
	vec3 undefP2 = gl_in[2].gl_Position.xyz;
	
	vec3 undefEdge0 = undefP1 - undefP0;
	vec3 undefEdge1 = undefP2 - undefP0;
	
	vec3 defP0 = i_deformedPos[0];
	vec3 defP1 = i_deformedPos[1];
	vec3 defP2 = i_deformedPos[2];
	
	vec3 defEdge0 = defP1 - defP0;
	vec3 defEdge1 = defP2 - defP0;
	
	float totalError = 0.;
	float sampleCount = 0.;
	
	const int edgeSections = 3;
	for(int i=0; i<=edgeSections; i++)
	{
		for(int j=0; j<=edgeSections; j++)
		{
			float u = float(i) / edgeSections;
			float v = float(j) / edgeSections;
			
			vec3 defP = defP0 + defEdge0 * u + defEdge1 * v;
			vec3 undefP = undefP0 + undefEdge0 * u + undefEdge1 * v;
			
			totalError += LinearizationError(undefP, defP);
			sampleCount++;
		}
	}
	
	float error = totalError / sampleCount;
	
	// find the correct tesselation level based on error
	float tessLevel = 1.;
	
	const float errorThresholds[4] = float[](
		0.05,
		0.1,
		0.15,
		0.2
	);
	
	for(int i=0; i<4; i++)
	{
		if(error > errorThresholds[i])
		{
			tessLevel = float(i + 2);
		}
		else
		{
			break;
		}
	}
	
	gl_TessLevelOuter[0] = tessLevel;
	gl_TessLevelOuter[1] = tessLevel;
	gl_TessLevelOuter[2] = tessLevel;
	gl_TessLevelInner[0] = tessLevel;
}