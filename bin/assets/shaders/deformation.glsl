
//#define KELVINLET_MODE
#define BONE_MODE

#ifdef KELVINLET_MODE
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletSharpness;
#endif

#ifdef BONE_MODE
#define MAX_BONES 3
struct BoneWeightVolume
{
	vec3 startPoint;
	vec3 direction;
	float length;
	float falloffRate;
};

uniform int u_bonesCount;
uniform BoneWeightVolume u_boneWeightVolumes[MAX_BONES];
uniform mat4 u_boneMatrices[MAX_BONES];
#endif

#ifdef KELVINLET_MODE
vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float alignment = max(dot(normalize(toPoint), normalize(force)), 0.);
	float displacement = pow(alignment, u_kelvinletSharpness);
	return displacement * force;
}
#endif

#ifdef BONE_MODE

float BoneWeight(vec3 point, int boneIndex)
{
	// calculate squared length to line segment defined by weight volume
	BoneWeightVolume bone = u_boneWeightVolumes[boneIndex];
	vec3 startToPoint = point - bone.startPoint;
	float projectionLength = clamp(dot(startToPoint, bone.direction), 0., bone.length);
	vec3 startToProjection = bone.direction * projectionLength;
	vec3 pointToProjection = startToProjection - startToPoint;
	float distanceSquared = dot(pointToProjection, pointToProjection);
	
	return 1./(1. + bone.falloffRate * distanceSquared * distanceSquared);
}

vec3 LinearBlend(vec3 point)
{	
	if(u_bonesCount == 0)
	{
		return point;
	}

	float weightSum = 0.;
	vec3 result = vec3(0.);
	
	for(int i=0; i<u_bonesCount; i++)
	{
		float weight = BoneWeight(point, i);
		result += weight * vec3(u_boneMatrices[i] * vec4(point, 1.));
		weightSum += weight;
	}
	
	return result * (1. / weightSum);
}
#endif

vec3 Deform(vec3 pos)
{
#ifdef KELVINLET_MODE
	return pos + Kelvinlet(
		pos, 
		u_localKelvinletCenter, 
		u_localKelvinletForce
	);
#endif
#ifdef BONE_MODE
	return LinearBlend(pos);
#endif
}