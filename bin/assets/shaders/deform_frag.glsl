#version 430

layout(location=0) in vec3 i_undeformedPos;

out vec4 o_color;
out float gl_FragDepth;

uniform int u_debug;
uniform mat3 u_N;
uniform mat4 u_MVP;
uniform vec3 u_localCameraPos;
uniform float u_pixelRadius;
uniform vec3 u_localKelvinletCenter;
uniform vec3 u_localKelvinletForce;

vec3 Kelvinlet(vec3 point, vec3 center, vec3 force) 
{
	vec3 toPoint = point - center;
	float dirCompare = dot(toPoint, force);
	
	if(dirCompare == 0.)
	{
		return vec3(0.);
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
	
	for(int i=0; i<16; i++)
	{
		vec3 k2 = UndeformedDirection(y1 + (0.5 * stepLength) * k1, defDirection);
		vec3 k3 = UndeformedDirection(y1 + (0.75 * stepLength) * k2, defDirection);
		vec3 y2 = y1 + stepLength * ((2. / 9.) * k1 + (1. / 3.) * k2 + (4. / 9.) * k3);
		vec3 k4 = UndeformedDirection(y2, defDirection);
		
		y1 += stepLength * ((7. / 24) * k1 + 0.25 * k2 + (1. / 3.) * k3 + (1. / 8.) * k4);
		k1 = k4;
		
		if(distance(y1, undefPoint) >= maxRadius)
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
	
	for(int i=0; i<16; i++)
	{
		y += k * stepLength;
		
		if(distance(y, undefPoint) >= maxRadius)
		{
			break;
		}
		
		k = UndeformedDirection(y, defDirection);
	}
	
	return y;
}

float SdBox(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.)) + min(max(q.x, max(q.y, q.z)), 0.);
}

float Sdf(vec3 p)
{
	float box = SdBox(p, vec3(1.)) - 0.1;
	float sphere = length(p) - 1.4;
	return min(box, sphere);
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
		
		float minRadius = u_pixelRadius * distTraveled * determinant(invJacobian);
		distTraveled += radius;
		
		if(radius < minRadius)
		{
			hit = true;
			break;
		}
		
		vec3 undefDirection = normalize(invJacobian * defDirection);
		
		if(radius < minRadius * 3.)
		{
			undefPoint = SolveEuler(undefPoint, defDirection, undefDirection, integrationStepLength, radius);
		}
		else
		{
			undefPoint = SolveBS23(undefPoint, defDirection, undefDirection, integrationStepLength, radius);
		}
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

vec3 Shade(vec3 undefHitPoint, vec3 defDirection, vec3 lightDir)
{
	vec3 worldNormal = WorldSdfGradient(undefHitPoint);
	vec3 worldDirection = normalize(u_N * defDirection);
	float diff = max(0., dot(worldNormal, -lightDir));
	float spec = pow(max(0., dot(reflect(worldDirection, worldNormal), -lightDir)), 32.);
	
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