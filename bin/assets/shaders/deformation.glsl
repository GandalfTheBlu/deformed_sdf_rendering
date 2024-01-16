
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
	const vec3 undefBonePoints[2] = vec3[](
		vec3(0.),
		vec3(0., 3., 0.)
	);
	
	// x: radius of effect, y: falloff slope (positive)
	const vec2 boneLinearFalloffs[2] = vec2[](
		vec2(4., 2.),
		vec2(8., 1.5)
	);

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
			0., -8., 0., 1.
		)
	);
	
	const float angle = u_kelvinletSharpness / 16. * 3.14159;
	const float cs = cos(angle);
	const float sn = sin(angle);
	const mat4 defBoneTransforms[2] = mat4[](
		mat4(
			1., 0., 0., 0.,
			0., 1., 0., 0.,
			0., 0., 1., 0.,
			1.5, 0., 0., 1.
		),
		mat4(
			cs, 0., -sn, 0.,
			0., 1., 0., 0.,
			sn, 0., cs, 0.,
			0., 8., 0., 1.
		)
	);
	
	vec3 displacement = vec3(0.);
	
	for(int i=0; i<2; i++)
	{	
		vec3 diff = point - undefBonePoints[i];
		float dist = sqrt(dot(diff, diff));
		vec2 falloff = boneLinearFalloffs[i];
		float boneWeight = max(0., falloff.x - dist * falloff.y) / falloff.x;
		
		// these matrices should be premultiplied on cpu
		vec4 defPoint = defBoneTransforms[i] * inverseUndefBoneTransforms[i] * vec4(point, 1.);
		
		
		displacement += boneWeight * (defPoint.xyz - point);
	}
	
	return point + displacement;
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