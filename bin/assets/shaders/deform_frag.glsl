#version 430

layout(location=0) in vec3 i_undeformedPos;

out vec4 o_color;

uniform vec3 u_color;
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;
uniform float u_kelvinletRadius;
uniform vec3 u_localCameraPos;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force, float radius) 
{
	vec3 toPoint = point - center;
    float distanceSquared = dot(toPoint, toPoint);
    float radiusSquared = radius * radius;
	
    if (distanceSquared > radiusSquared) 
	{
        return vec3(0.);
    }
	
    vec3 displacement = (
        3. * dot(toPoint, force) * (radiusSquared - distanceSquared) /
        (4. * 3.14159 * distanceSquared * distanceSquared)
    ) * toPoint;

    return displacement;
}

vec3 Deform(vec3 pos)
{
	return pos + Kelvinlet(
		pos, 
		u_localKelvinletCenter, 
		u_localKelvinletForce, 
		u_kelvinletRadius
	);
}

mat3 DeformationGradient(vec3 undefPoint)
{	
	// calculate the gradients in the x-, y-, and z-planes using
	// the central differance and construct the Jacobian matrix
	const vec2 diff = vec2(0.0001, 0.);// dx = dy = dz = 0.0001
	const float scale = 5000.;// = 1 / (2 * 0.0001)
	
	vec3 gradientX = (Deform(undefPoint + diff.xyy) - Deform(undefPoint - diff.xyy)) * scale;
	vec3 gradientY = (Deform(undefPoint + diff.yxy) - Deform(undefPoint - diff.yxy)) * scale;
	vec3 gradientZ = (Deform(undefPoint + diff.yyx) - Deform(undefPoint - diff.yyx)) * scale;
	
	return mat3(gradientX, gradientY, gradientZ);
}

float Sdf(vec3 undefPoint)
{
	return length(undefPoint) - 1.;
}

vec3 NLST(vec3 undefOrigin, vec3 undefDirection, inout bool hit)
{
	// non-linear sphere tracing:
	// perform ODE traversal inside each unbounding-sphere using the deformation jacobian
	// as the ray direction
	const float substep = 0.03;
	vec3 undefPoint = undefOrigin;
	hit = false;
	
	for(int i=0; i<32; i++)
	{
		vec3 sphereCenter = undefPoint;
		float radius = Sdf(sphereCenter);
		
		if(radius < 0.01)
		{
			hit = true;
			break;
		}
		
		for(int j=0; j<64; j++)
		{
			vec3 invDefDirection = normalize(inverse(DeformationGradient(undefPoint)) * undefDirection);
			undefPoint += invDefDirection * substep;
			
			if(distance(undefPoint, sphereCenter) >= radius)
			{
				break;
			}
		}
	}
	
	return undefPoint;
}

void main()
{
	vec3 undefOrigin = i_undeformedPos;
	vec3 undefDirection = normalize(undefOrigin - u_localCameraPos);
	
	bool hit = false;
	vec3 undefHitPoint = NLST(undefOrigin, undefDirection, hit);
	
	if(!hit)
	{
		discard;
	}
	
	vec3 color = abs(cos(Deform(undefHitPoint) * 10.));
	
	//vec3 color = vec3(abs(undefOrigin.z));
	
	o_color = vec4(color, 1.);
}