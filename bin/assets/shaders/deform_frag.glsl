#version 430
#include "assets/shaders/deformation.glsl"

layout(location=0) in vec3 i_undeformedPos;

in vec4 gl_FragCoord;

out vec4 o_color;
out float gl_FragDepth;

// debug settings
uniform int u_renderMode;// 0 = sphere trace, 1 = mesh
uniform int u_boneIndex;// used to color based on weight (if not -1)

uniform mat3 u_N;
uniform mat4 u_MVP;
uniform vec3 u_localCameraPos;
uniform float u_pixelRadius;
uniform vec2 u_screenSize;
layout(binding=0) uniform sampler2D u_backfaceDistanceTexture;

mat3 DeformationJacobian(vec3 undefPoint)
{	
	// calculate the gradients in the x-, y-, and z-planes using
	// the forward differences and construct the Jacobian matrix
	const vec2 diff = vec2(0.0001, 0.);// dx = dy = dz = 0.0001
	const float scale = 10000.;// = 1 / 0.0001
	
	vec3 centerDeformation = Deform(undefPoint);
	mat3 jacobian = mat3(0.);
	jacobian[0] = (Deform(undefPoint + diff.xyy) - centerDeformation) * scale;
	jacobian[1] = (Deform(undefPoint + diff.yxy) - centerDeformation) * scale;
	jacobian[2] = (Deform(undefPoint + diff.yyx) - centerDeformation) * scale;
	
	return jacobian;
}

vec3 UndeformedDirection(vec3 undefPoint, vec3 defDirection)
{
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	return normalize(invJacobian * defDirection);
}

vec3 SolveBS23(
	vec3 undefPoint, 
	vec3 defDirection, 
	vec3 startUndefDirection, 
	float maxRadius, 
	inout float defDistTraveled
)
{
	// Bogacki-Shampine ode solver that returns the next undeformed point along the 
	// deformed ray path
	const float stepLength = 0.2 / 16.;
	const vec3 startUndefPoint = undefPoint;
	const float maxRadiusSquared = maxRadius * maxRadius;
	vec3 y1 = undefPoint;
	vec3 k1 = startUndefDirection;
	
	for(int i=0; i<16; i++)
	{	
		vec3 k2 = UndeformedDirection(y1 + (0.5 * stepLength) * k1, defDirection);
		vec3 k3 = UndeformedDirection(y1 + (0.75 * stepLength) * k2, defDirection);
		vec3 y2 = y1 + stepLength * ((2. / 9.) * k1 + (1. / 3.) * k2 + (4. / 9.) * k3);
		vec3 k4 = UndeformedDirection(y2, defDirection);
		
		y1 += stepLength * ((7. / 24) * k1 + 0.25 * k2 + (1. / 3.) * k3 + (1. / 8.) * k4);
		k1 = k4;
		
		defDistTraveled += stepLength;
		vec3 centerOffset = y1 - startUndefPoint;
		float offsetSquared = dot(centerOffset, centerOffset);
		
		if(offsetSquared >= maxRadiusSquared)
		{
			break;
		}
	}
	
	return y1;
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

float Sdf(vec3 p)
{
	float torso = Capsule(p, vec3(0., -0.5, 0.), vec3(0., 0.5, 0.), 0.25);
	vec3 mirroredP = vec3(abs(p.x), p.y, p.z);
	float arms = Capsule(mirroredP, vec3(0.25, 0.5, 0.), vec3(0.75, 0.5, 0.), 0.25);
	float legs = Capsule(mirroredP, vec3(0.25, -0.5, 0.), vec3(0.25, -1., 0.), 0.25);

	return min(torso, min(arms, legs));
}

float OffsetError(float defDistTraveled)
{
	return 0.001 * defDistTraveled;
}

vec3 AdjustTerminationPoint(vec3 undefTerminPoint, vec3 undefTerminDirection, float defDistTraveled)
{
	float offset = 0.;
	
	for(int i=0; i<3; i++)
	{
		offset += 
			Sdf(undefTerminPoint + undefTerminDirection * offset) - 
			OffsetError(defDistTraveled + offset);
	}
	
	return undefTerminPoint + undefTerminDirection * offset;
}

vec3 NLST(vec3 undefOrigin, vec3 defDirection, float toOriginDistance, float maxDefDistance, inout bool hit)
{
	// non-linear sphere tracing:
	// perform ODE traversal inside each unbounding-sphere using the deformation jacobian
	// to transform the ray direction
	float defDistTraveled = toOriginDistance;
	vec3 undefPoint = undefOrigin;
	vec3 undefDirection = vec3(0.);
	hit = false;
	
	for(int i=0; i<64; i++)
	{
		float radius = Sdf(undefPoint);
		
		mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
		
		float minRadius = u_pixelRadius * defDistTraveled * determinant(invJacobian);
		
		undefDirection = normalize(invJacobian * defDirection);
		
		if(radius < minRadius)
		{
			hit = true;
			break;
		}
		if(defDistTraveled > maxDefDistance)
		{
			break;
		}
		
		if(radius < minRadius * 3.)
		{
			// simple euler integration step
			undefPoint += undefDirection * radius;
			defDistTraveled += radius;
		}
		else
		{
			undefPoint = SolveBS23(
				undefPoint, 
				defDirection, 
				undefDirection,  
				radius, 
				defDistTraveled
			);
		}
	}
	
	if(hit)
	{
		undefPoint = AdjustTerminationPoint(
			undefPoint,
			undefDirection,
			defDistTraveled
		);		
	}
	
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

vec3 BoneWeightColor(vec3 undefPoint)
{
	float weight = BoneWeight(undefPoint, u_boneIndex);
	
	return vec3(weight, 0., 0.);
}

vec3 Shade(vec3 undefHitPoint, vec3 defDirection, vec3 lightDir)
{
	vec3 worldNormal = WorldSdfGradient(undefHitPoint);
	vec3 worldDirection = normalize(u_N * defDirection);
	float diff = max(0., dot(worldNormal, -lightDir));
	float spec = pow(max(0., dot(reflect(worldDirection, worldNormal), -lightDir)), 32.);
	
	vec3 ambientColor = vec3(0.1, 0.1, 0.2);
	vec3 lightColor = vec3(1., 0.9, 0.7);
	vec3 albedo = vec3(0.);
	
	if(u_boneIndex != -1)
	{
		albedo = BoneWeightColor(undefHitPoint);
	}
	else
	{
		albedo = DeformationColor(undefHitPoint);
	}
	
	
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
		float maxDefDistance = texture(u_backfaceDistanceTexture, gl_FragCoord.xy / u_screenSize).r;
		vec3 undefOrigin = i_undeformedPos;
		vec3 defDirection = Deform(undefOrigin) - u_localCameraPos;
		float toOriginDistance = length(defDirection);
		defDirection *= 1. / toOriginDistance;
		
		bool hit = false;
		vec3 undefHitPoint = NLST(
			undefOrigin, 
			defDirection, 
			toOriginDistance, 
			maxDefDistance, 
			hit
		);
		
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
		o_color = vec4(0.25, 0.25, 0.25, 1.);
		gl_FragDepth = DeformedPointToDepth(Deform(i_undeformedPos));
	}
}