#version 430
#include "assets/shaders/deformation.glsl"

layout(location=0) in vec3 i_undeformedPos;

out vec4 o_color;
out float gl_FragDepth;

// debug settings
uniform int u_renderMode;// 0 = sphere trace, 1 = default (mesh)

uniform mat3 u_N;
uniform mat4 u_MVP;
uniform vec3 u_localCameraPos;
//uniform float u_pixelRadius;

mat3 DeformationJacobian(vec3 undefPoint)
{	
	// calculate the gradients in the x-, y-, and z-planes using
	// the central differance and construct the Jacobian matrix
	const vec2 diff = vec2(0.0001, 0.);// dx = dy = dz = 0.0001
	const float scale = 5000.;// = 1 / (2 * 0.0001)
	
#ifdef BONE_MODE
	FindCurrentBones(undefPoint);
#endif	
	
	vec3 gradientX = (Deform(undefPoint + diff.xyy) - Deform(undefPoint - diff.xyy)) * scale;
	vec3 gradientY = (Deform(undefPoint + diff.yxy) - Deform(undefPoint - diff.yxy)) * scale;
	vec3 gradientZ = (Deform(undefPoint + diff.yyx) - Deform(undefPoint - diff.yyx)) * scale;
	
	return mat3(gradientX, gradientY, gradientZ);
}

vec3 UndeformedDirection(vec3 undefPoint, vec3 defDirection)
{
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	return normalize(invJacobian * defDirection);
}

vec3 SolveBS23(vec3 undefPoint, vec3 defDirection, vec3 startUndefDirection, float stepLength, float maxRadius)
{
	// Bogacki-Shampine ode solver that returns the next undeformed point along the 
	// deformed ray path
	
	vec3 y1 = undefPoint;
	vec3 k1 = startUndefDirection;
	
	float distanceTraveled = 0.;
	
	for(int i=0; i<16; i++)
	{	
		vec3 k2 = UndeformedDirection(y1 + (0.5 * stepLength) * k1, defDirection);
		vec3 k3 = UndeformedDirection(y1 + (0.75 * stepLength) * k2, defDirection);
		vec3 y2 = y1 + stepLength * ((2. / 9.) * k1 + (1. / 3.) * k2 + (4. / 9.) * k3);
		vec3 k4 = UndeformedDirection(y2, defDirection);
		
		y1 += stepLength * ((7. / 24) * k1 + 0.25 * k2 + (1. / 3.) * k3 + (1. / 8.) * k4);
		k1 = k4;
		
		distanceTraveled += stepLength;
		
		if(distanceTraveled >= maxRadius)
		{
			break;
		}
	}
	
	return y1;
}

vec3 SolveEuler(vec3 undefPoint, vec3 defDirection, vec3 startUndefDirection, float stepLength, float maxRadius)
{
	// Euler integration for stepping along the deformed ray path when the surface offset is small
	
	vec3 y = undefPoint;
	vec3 k = startUndefDirection;
	
	float distanceTraveled = 0.;
	
	for(int i=0; i<16; i++)
	{
		y += k * stepLength;
		
		distanceTraveled += stepLength;
		
		if(distanceTraveled >= maxRadius)
		{
			break;
		}
		
		k = UndeformedDirection(y, defDirection);
	}
	
	return y;
}

vec3 Fold(vec3 p, vec3 normal)
{
	return p - 2. * min(0., dot(p, normal)) * normal;
}

vec3 RotX(vec3 p, float angle)
{
	float startAngle = atan(p.y, p.z);
	float radius = length(p.yz);
	return vec3(p.x, radius * sin(startAngle + angle), radius * cos(startAngle + angle));
}

float Capsule(vec3 p, vec3 a, vec3 b, float radius)
{
	vec3 pa = p - a;
	vec3 ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
	return length(pa - ba * h) - radius;
}

float Box(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.)) + min(max(q.x, max(q.y, q.z)), 0.);
}

float Tree(vec3 p)
{	
	vec2 dim = vec2(1., 8.);
	float d = Capsule(p, vec3(0., -1., 0.), vec3(0., 1. + dim.y, 0.), dim.x);
	vec3 scale = vec3(1.);
	vec3 change = vec3(0.7,0.68,0.7);
	
	const vec3 n1 = normalize(vec3(1., 0., 1.)); 
	const vec3 n2 = vec3(n1.x, 0., -n1.z);
	const vec3 n3 = vec3(-n1.x, 0., n1.z);
	
	for(int i=0; i<7; i++)
	{
		p = Fold(p, n1);	
		p = Fold(p, n2);	
		p = Fold(p, n3);	
		
		p.y -= scale.y*dim.y;
		p.z = abs(p.z);
		p = RotX(p, 3.1415*0.25);
		
		scale *= change;
		
		d = min(d, Capsule(p, vec3(0.), vec3(0., dim.y * scale.y, 0.), scale.x*dim.x));
	}
	
	return d;
}

float Sdf(vec3 p)
{
	return Box(p, vec3(0.5, 2., 0.5));
	//return Tree(p * 5.) / 5.;
}

float OffsetError(float distanceTraveled)
{
	return 0.001 * distanceTraveled;
}

vec3 AdjustTerminationPoint(vec3 undefTerminPoint, vec3 undefTerminDirection, float distanceTraveled)
{
	float offset = 0.;
	
	for(int i=0; i<3; i++)
	{
		offset += 
			Sdf(undefTerminPoint + undefTerminDirection * offset) - 
			OffsetError(distanceTraveled + offset);
	}
	
	return undefTerminPoint + undefTerminDirection * offset;
}

vec3 NLST(vec3 undefOrigin, vec3 defDirection, float toOriginDistance, inout bool hit)
{
	// non-linear sphere tracing:
	// perform ODE traversal inside each unbounding-sphere using the deformation jacobian
	// to transform the ray direction
	const float integrationStepLength = 0.2 / 16.;
	float distTraveled = toOriginDistance;
	vec3 undefPoint = undefOrigin;
	hit = false;
	
	for(int i=0; i<32; i++)
	{
		vec3 sphereCenter = undefPoint;
		float radius = Sdf(sphereCenter);
		
		mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
		
		//float minRadius = u_pixelRadius * distTraveled * determinant(invJacobian);
		distTraveled += radius;
		
		if(radius < 0.001/*minRadius*/)
		{
			hit = true;
			break;
		}
		
		vec3 undefDirection = normalize(invJacobian * defDirection);
		
		undefPoint = SolveEuler(undefPoint, defDirection, undefDirection, integrationStepLength, radius);
		
		//if(radius < 0.01/*minRadius * 3.*/)
		//{
		//	undefPoint = SolveEuler(undefPoint, defDirection, undefDirection, integrationStepLength, radius);
		//}
		//else
		//{
		//	undefPoint = SolveBS23(undefPoint, defDirection, undefDirection, integrationStepLength, radius);
		//}
	}
	
	undefPoint = AdjustTerminationPoint(
		undefPoint,
		UndeformedDirection(undefPoint, defDirection),
		distTraveled
	);
	
	return undefPoint;
}

vec3 WorldSdfGradient(vec3 undefPoint)
{
	const float step = 0.0001;
    const vec2 diff = vec2(1., -1.);
    vec3 undefGradient = diff.xyy*Sdf(undefPoint + diff.xyy * step) + 
					diff.yyx*Sdf(undefPoint + diff.yyx * step) + 
					diff.yxy*Sdf(undefPoint + diff.yxy * step) + 
					diff.xxx*Sdf(undefPoint + diff.xxx * step);
					
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	vec3 defGradient = transpose(invJacobian) * undefGradient;
	return normalize(u_N * defGradient);
}

vec3 DeformationColor(vec3 undefPoint)
{
	const float spectrumScale = 2.;
	float deformAmount = distance(undefPoint, Deform(undefPoint));
	deformAmount = min(deformAmount / spectrumScale, 1.);
	
	const vec3 blue = vec3(0.2, 0., 1.);
	const vec3 green = vec3(0., 1., 0.2);
	const vec3 yellow = vec3(1., 1., 0.);
	const vec3 red = vec3(1., 0., 0.);
	
	vec3 color = mix(blue, green, smoothstep(0., 0.33, deformAmount));
	color = mix(color, yellow, smoothstep(0.33, 0.66, deformAmount));
	color = mix(color, red, smoothstep(0.66, 1., deformAmount));
	return color;
}

vec3 Shade(vec3 undefHitPoint, vec3 defDirection, vec3 lightDir)
{
	vec3 worldNormal = WorldSdfGradient(undefHitPoint);
	vec3 worldDirection = normalize(u_N * defDirection);
	float diff = max(0., dot(worldNormal, -lightDir));
	float spec = pow(max(0., dot(reflect(worldDirection, worldNormal), -lightDir)), 32.);
	
	vec3 ambientColor = vec3(0.1, 0.1, 0.2);
	vec3 lightColor = vec3(1., 0.9, 0.7);
	vec3 albedo = DeformationColor(undefHitPoint);
	
	return albedo * (ambientColor + lightColor * (diff + spec));
}

float DeformedPointToDepth(vec3 defPoint)
{
	vec4 clipPoint = u_MVP * vec4(defPoint, 1.);
	return (clipPoint.z / clipPoint.w + 1.) * 0.5;
}

void main()
{
	if(u_renderMode == 0)
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
		o_color = vec4(Shade(undefHitPoint, defDirection, lightDir), 1.);
		
		gl_FragDepth = DeformedPointToDepth(Deform(undefHitPoint));
	}
	else
	{
		o_color = vec4(0.25);
		
		FindCurrentBones(i_undeformedPos);
		gl_FragDepth = DeformedPointToDepth(Deform(i_undeformedPos));
	}
}