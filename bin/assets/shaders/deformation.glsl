
//#define KELVINLET_MODE
#define BONE_MODE

#ifdef KELVINLET_MODE
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletSharpness;
#endif

#ifdef BONE_MODE
#define MAX_BONES 2
struct CapsulePrimitive
{
	vec3 startPoint;
	vec3 endPoint;
	float radius;
};

uniform int u_bonesCount;
uniform CapsulePrimitive u_boneWeightVolumes[MAX_BONES];
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
int g_boneIndex1 = -1;
int g_boneIndex2 = -1;
float g_boneWeight1 = 0.;
float g_boneWeight2 = 0.;

float BoneWeight(vec3 p, CapsulePrimitive c)
{
	vec3 pa = p - c.startPoint;
	vec3 ba = c.endPoint - c.startPoint;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
	vec3 v = pa - ba * h;
	float d = dot(v, v) - c.radius * c.radius;
	
	return max(0., 1. - max(0., d));
}

void FindCurrentBones(vec3 point)
{
	g_boneIndex1 = -1;
	g_boneIndex2 = -1;
	g_boneWeight1 = 0.;
	g_boneWeight2 = 0.;
	
	for(int i=0; i<u_bonesCount; i++)
	{
		float weight = BoneWeight(point, u_boneWeightVolumes[i]);
		
		if(weight > g_boneWeight1)
		{
			g_boneIndex2 = g_boneIndex1;
			g_boneWeight2 = g_boneWeight1;
			
			g_boneIndex1 = i;
			g_boneWeight1 = weight;
		}
		else if(weight > g_boneWeight2)
		{
			g_boneIndex2 = i;
			g_boneWeight2 = weight;
		}
	}
}

vec3 LinearBlend(vec3 point)
{	
	if(g_boneIndex1 != -1)
	{
		vec3 deformed = vec3(u_boneMatrices[g_boneIndex1] * vec4(point, 1.));
		
		if(g_boneIndex2 != -1)
		{
			deformed = g_boneWeight1 * deformed + 
				g_boneWeight2 * vec3(u_boneMatrices[g_boneIndex2] * vec4(point, 1.));
			
			deformed *= 1. / (g_boneWeight1 + g_boneWeight2);
		}
		
		return deformed;
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