#version 430
#include "assets/shaders/deformation.glsl"

layout(vertices=3) out;

layout(location=0) in vec3 i_deformedPos[];

float LinearizationError(vec3 undeformedPos, vec3 deformedPos)
{
	// differance between the interpolated deformed position and the deformed interpolated position
	return distance(deformedPos, Deform(undeformedPos));
}

float GetTessellationLevel(float error)
{
	if(error > 0.04)
	{
		return 5.;
	}
	if(error > 0.03)
	{
		return 4.;
	}
	if(error > 0.02)
	{
		return 3.;
	}
	if(error > 0.01)
	{
		return 2.;
	}
	return 1.;
}

void main()
{
	// pass the undeformed vertex to the tessellation evaluation shader
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	
	// only one vertex per patch handles tesselation
	if(gl_InvocationID != 0)
	{
		return;
	}
	
	// calculate the linearization error across each edge
	// to get the same result for neighboring triangles
	
	// edge 0: start = 1, end = 2
	// edge 1: start = 0, end = 2
	// edge 2: start = 0, end = 1
	const int edgeStartIndices[3] = int[]
	(
		1, 0, 0
	);
	const int edgeEndIndices[3] = int[]
	(
		2, 2, 1
	);
	const int edgeSections = 5;
	const float invEdgeSections = 1. / float(edgeSections);
	
	float edgeErrors[3] = float[](
		0., 0., 0.
	);
	
	for(int i=0; i<3; i++)
	{
		int startIndex = edgeStartIndices[i];
		int endIndex = edgeEndIndices[i];
		
		vec3 undefEdgeStart = gl_in[startIndex].gl_Position.xyz;
		vec3 undefEdgeEnd = gl_in[endIndex].gl_Position.xyz;
		vec3 undefEdge = undefEdgeEnd - undefEdgeStart;
		
		vec3 defEdgeStart = i_deformedPos[startIndex];
		vec3 defEdgeEnd = i_deformedPos[endIndex];
		vec3 defEdge = defEdgeEnd - defEdgeStart;
		
		for(int j=0; j<=edgeSections; j++)
		{
			float edgeScale = float(j) * invEdgeSections;
			
			vec3 undefPoint = undefEdgeStart + undefEdge * edgeScale;
			vec3 defPoint = defEdgeStart + defEdge * edgeScale;
			
			edgeErrors[i] += LinearizationError(undefPoint, defPoint);
		}
		
		edgeErrors[i] *= invEdgeSections;
	}
	
	// use the average edge error for the center tessellation
	float averageError = 
		(edgeErrors[0] + edgeErrors[1] + edgeErrors[2]) / 3.;
	
	gl_TessLevelOuter[0] = GetTessellationLevel(edgeErrors[0]);
	gl_TessLevelOuter[1] = GetTessellationLevel(edgeErrors[1]);
	gl_TessLevelOuter[2] = GetTessellationLevel(edgeErrors[2]);
	gl_TessLevelInner[0] = GetTessellationLevel(averageError);
}