
#define MAX_JOINTS 16
struct JointWeightVolume
{
	vec3 startPoint;
	vec3 direction;
	float length;
	float falloffRate;
};

uniform int u_jointCount;
uniform JointWeightVolume u_jointWeightVolumes[MAX_JOINTS];
uniform mat4 u_deformationMatrices[MAX_JOINTS];

/*vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float alignment = max(dot(normalize(toPoint), normalize(force)), 0.);
	float displacement = pow(alignment, u_kelvinletSharpness);
	return displacement * force;
}*/

float JointWeight(vec3 point, int jointIndex)
{
	// calculate squared length to line segment defined by weight volume
	JointWeightVolume joint = u_jointWeightVolumes[jointIndex];
	vec3 startToPoint = point - joint.startPoint;
	float projectionLength = clamp(dot(startToPoint, joint.direction), 0., joint.length);
	vec3 startToProjection = joint.direction * projectionLength;
	vec3 pointToProjection = startToProjection - startToPoint;
	float distanceSquared = dot(pointToProjection, pointToProjection);
	
	return 1./(1. + joint.falloffRate * distanceSquared * distanceSquared);
}

vec3 LinearBlend(vec3 point)
{	
	if(u_jointCount == 0)
	{
		return point;
	}

	float weightSum = 0.;
	vec3 result = vec3(0.);
	
	for(int i=0; i<u_jointCount; i++)
	{
		float weight = JointWeight(point, i);
		result += weight * vec3(u_deformationMatrices[i] * vec4(point, 1.));
		weightSum += weight;
	}
	
	return result * (1. / weightSum);
}

vec3 Deform(vec3 pos)
{
	return LinearBlend(pos);
}