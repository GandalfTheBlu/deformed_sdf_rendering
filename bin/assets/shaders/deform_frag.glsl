#version 430

layout(location=0) in vec3 i_undeformedPos;

out vec4 o_color;
out float gl_FragDepth;

uniform int u_debug;
uniform vec3 u_color;
uniform vec3 u_localCameraPos;
uniform float u_pixelRadius;
uniform mat4 u_MVP;
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float dirCompare = dot(toPoint, force);
	
	if(dirCompare <= 0.)
	{
		//return vec3(0.);
	}
	
	float displacement = exp(-(dot(toPoint, toPoint) - 
		dirCompare * dirCompare / dot(force, force)));
		
	return force * displacement;
}

vec3 Deform(vec3 pos)
{
	return pos + Kelvinlet(
		pos, 
		u_localKelvinletCenter, 
		u_localKelvinletForce
	);
}

mat3 DeformationJacobian(vec3 undefPoint)
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

float SdBox(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.)) + min(max(q.x, max(q.y, q.z)), 0.);
}

float Sdf(vec3 p)
{
	return SdBox(p, vec3(1.));
}

vec3 NLST(vec3 undefOrigin, vec3 defDirection, float toOriginDistance, inout bool hit)
{
	// non-linear sphere tracing:
	// perform ODE traversal inside each unbounding-sphere using the deformation jacobian
	// to transform the ray direction
	const float substep = 0.1 / 32.;
	float distTraveled = toOriginDistance;
	vec3 undefPoint = undefOrigin;
	hit = false;
	
	for(int i=0; i<32; i++)
	{
		vec3 sphereCenter = undefPoint;
		float radius = Sdf(sphereCenter);
		
		mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
		
		float minRadius = u_pixelRadius * distTraveled * determinant(invJacobian);
		distTraveled += radius;
		
		if(radius < minRadius)
		{
			hit = true;
			break;
		}
		
		for(int j=0; j<32; j++)
		{
			vec3 undefDirection = normalize(invJacobian * defDirection);
			undefPoint += undefDirection * substep;
		
			if(distance(undefPoint, sphereCenter) >= radius)
			{
				break;
			}
			
			invJacobian = inverse(DeformationJacobian(undefPoint));
		}
	}
	
	return undefPoint;
}

vec3 DeformedSdfGradient(vec3 undefPoint)
{
	const float step = 0.0001;
    const vec2 diff = vec2(1., -1.);
    vec3 undefGradient = diff.xyy*Sdf(undefPoint + diff.xyy * step) + 
					diff.yyx*Sdf(undefPoint + diff.yyx * step) + 
					diff.yxy*Sdf(undefPoint + diff.yxy * step) + 
					diff.xxx*Sdf(undefPoint + diff.xxx * step);
					
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	return normalize(transpose(invJacobian) * undefGradient);
}

vec3 Shade(vec3 undefHitPoint, vec3 defDirection, vec3 lightDir)
{
	vec3 defNormal = DeformedSdfGradient(undefHitPoint);
	float diff = max(0., dot(defNormal, -lightDir));
	float spec = pow(max(0., dot(reflect(defDirection, defNormal), -lightDir)), 32.);
	
	vec3 ambientColor = vec3(0.1, 0.1, 0.2);
	vec3 lightColor = vec3(1., 0.9, 0.7);
	vec3 albedo = vec3(1.);
	
	return albedo * (ambientColor + lightColor * (diff + spec));
}

float DeformedPointToDepth(vec3 defPoint)
{
	vec4 clipPoint = u_MVP * vec4(defPoint, 1.);
	return (clipPoint.z / clipPoint.w + 1.) * 0.5;
}

void main()
{
	if(u_debug == 0)
	{
		vec3 undefOrigin = i_undeformedPos;
		vec3 defDirection = Deform(undefOrigin) - u_localCameraPos;
		float toOriginDistance = length(defDirection);
		defDirection *= 1. / toOriginDistance;
		
		bool hit = false;
		vec3 undefHitPoint = NLST(undefOrigin, defDirection, toOriginDistance, hit);
		
		if(!hit)
		{
			discard;
		}
		
		vec3 lightDir = normalize(vec3(-0.8, -1., 0.6));
		vec3 color = Shade(undefHitPoint, defDirection, lightDir);
		
		o_color = vec4(color, 1.);
		gl_FragDepth = DeformedPointToDepth(Deform(undefHitPoint));
	}
	else
	{
		o_color = vec4(0.25);
		gl_FragDepth = DeformedPointToDepth(Deform(i_undeformedPos));
	}
}