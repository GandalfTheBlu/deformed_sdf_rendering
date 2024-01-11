#version 430

layout(vertices=3) out;

layout(location=0) in vec3 i_deformedPos[];

uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float dirCompare = dot(toPoint, force);
	
	if(dirCompare <= 0.)
	{
		//return vec3(0.);
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
	
	const int edgeSections = 5;
	for(int i=0; i<=edgeSections; i++)
	{
		float u = float(i) / edgeSections;
		vec3 undefEdgePoint0 = undefP0 + undefEdge0 * u;
		vec3 undefEdgePoint1 = undefP1 + undefEdge1 * u;
		vec3 undefFacePath = undefEdgePoint1 - undefEdgePoint0;
		vec3 defEdgePoint0 = defP0 + defEdge0 * u;
		vec3 defEdgePoint1 = defP1 + defEdge1 * u;
		vec3 defFacePath = defEdgePoint1 - defEdgePoint0;
		
		for(int j=0; j<=edgeSections; j++)
		{
			float v = float(j) / edgeSections;
			vec3 undefFacePoint = undefEdgePoint0 + undefFacePath * v;
			vec3 defFacePoint = defEdgePoint0 + defFacePath * v;
			
			totalError += LinearizationError(undefFacePoint, defFacePoint);
			sampleCount++;
		}
	}
	
	float error = totalError / sampleCount;
	
	// find the correct tesselation level based on error
	float tessLevel = 1.;
	
	const float errorThresholds[4] = float[](
		0.01,
		0.02,
		0.03,
		0.04
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