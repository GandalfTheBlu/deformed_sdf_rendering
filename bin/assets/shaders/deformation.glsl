
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletSharpness;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float alignment = max(dot(normalize(toPoint), normalize(force)), 0.);
	float displacement = pow(alignment, u_kelvinletSharpness);
	return displacement * force;
}

vec3 LinearBlend(vec3 point)
{	
	const mat4 inverseUndefBoneTransforms[2] = mat4[](
		mat4(
			1., 0., 0., 0.,
			0., 1., 0., 0.,
			0., 0., 1., 0.,
			0., 0., 0., 1.
		),
		mat4(
			1., 0., 0., 0.,
			0., 1., 0., 0.,
			0., 0., 1., 0.,
			0., -4., 0., 1.
		)
	);
	
	const mat4 defBoneTransforms[2] = mat4[](
		mat4(
			1., 0., 0., 0.,
			0., 2., 0., 0.,
			0., 0., 1., 0.,
			0., 0., 0., 1.
		),
		mat4(
			1., 0., 0., 0.,
			0., 1., 0., 0.,
			0., 0., 1., 0.,
			2., 4., 0., 1.
		)
	);
	
	vec3 result = vec3(0.);
	
	for(int i=0; i<2; i++)
	{
		// use distance falloff function some way instead
		float boneWeight = 0.5;
		
		// point relative to bone's default transform
		vec4 relPoint = inverseUndefBoneTransforms[i] * vec4(point, 1.);
		
		// point in world space, deformed by bone
		vec4 defPoint = defBoneTransforms[i] * relPoint;
		
		result += boneWeight * defPoint.xyz;
	}
	
	return result;
}

vec3 Deform(vec3 pos)
{
	//return pos + Kelvinlet(
	//	pos, 
	//	u_localKelvinletCenter, 
	//	u_localKelvinletForce
	//);
	
	return LinearBlend(pos);
}