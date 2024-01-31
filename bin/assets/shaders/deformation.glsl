
//#define KELVINLET_MODE
#define BONE_MODE

#ifdef KELVINLET_MODE
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletSharpness;
#endif

#ifdef BONE_MODE
#define MAX_BONES 2
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
	float weight1 = 0.;
	float weight2 = 0.;
	int boneIndex1 = -1;
	int boneIndex2 = -1;

	for(int i=0; i<u_bonesCount; i++)
	{
		float weight = BoneWeight(point, i);
		if(weight > weight1)
		{
			weight2 = weight1;
			boneIndex2 = boneIndex1;
			weight1 = weight;
			boneIndex1 = i;
		}
		else if(weight > weight2)
		{
			weight2 = weight;
			boneIndex2 = i;
		}
	}

	float wSum = weight1 + weight2;
	
	if(wSum == 0.)
	{
		return point;
	}
	
	return (weight1 * vec3(u_boneMatrices[boneIndex1] * vec4(point, 1.)) + 
			weight2 * vec3(u_boneMatrices[boneIndex2] * vec4(point, 1.))) * 
			(1. / wSum);
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