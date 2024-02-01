
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
	vec3 startToEnd;
	float lengthSquared;
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
float CheapExp(float x) 
{
    x = 1. + x / 256.;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    return x;
}

float BoneWeight(vec3 p, int boneIndex)
{
	BoneWeightVolume bone = u_boneWeightVolumes[boneIndex];
	vec3 startToPoint = p - bone.startPoint;
	float h = clamp(dot(startToPoint, bone.startToEnd) / bone.lengthSquared, 0., 1.);
	vec3 v = startToPoint - bone.startToEnd * h;
	float d = dot(v, v);
	
	return CheapExp(-bone.falloffRate*d);
}

vec3 LinearBlend(vec3 point)
{	
	float weightSum = 0.;
	vec3 result = vec3(0.);
	
	for(int i=0; i<u_bonesCount; i++)
	{
		float weight = BoneWeight(point, i);
		result += weight * vec3(u_boneMatrices[i] * vec4(point, 1.));
		weightSum += weight;
	}
	
	if(weightSum > 0.)
	{
		return result * (1. / weightSum);
	}
	
	return point;
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