
//#define KELVINLET_MODE
#define BONE_MODE

#ifdef KELVINLET_MODE
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletSharpness;
#endif

#ifdef BONE_MODE
#define BONE_COUNT 2
uniform vec3 u_undeformedBonePoints[BONE_COUNT];
uniform vec2 u_boneFalloffs[BONE_COUNT];// x: radius of effect, y: falloff rate
uniform mat4 u_boneMatrices[BONE_COUNT];
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
vec3 LinearBlend(vec3 point)
{	
	vec3 displacement = vec3(0.);
	
	for(int i=0; i<BONE_COUNT; i++)
	{	
		vec3 diff = point - u_undeformedBonePoints[i];
		vec2 falloff = u_boneFalloffs[i];
		float dist = max(0., sqrt(dot(diff, diff)) - falloff.x);
		float boneWeight = exp(-dist * falloff.y);
		
		vec4 defPoint = u_boneMatrices[i] * vec4(point, 1.);
		
		
		displacement += boneWeight * (defPoint.xyz - point);
	}
	
	return point + displacement;
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