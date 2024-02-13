#version 430
#include "assets/shaders/deformation.glsl"
#include "assets/shaders/sdf.glsl"

layout(location=0) in vec3 i_undeformedPos;

in vec4 gl_FragCoord;

out vec4 o_color;
out float gl_FragDepth;

// debug settings
uniform int u_renderMode;// 0 = sphere trace, 1 = mesh
uniform int u_jointIndex;// used to color based on weight (if not -1)

uniform mat4 u_VP;
uniform mat4 u_invVP;
uniform vec3 u_cameraPos;
uniform float u_pixelRadius;
uniform vec2 u_screenSize;
uniform float u_maxDistanceFromSurface;
uniform float u_maxRadius;

mat3 DeformationJacobian(const vec3 undefPoint)
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

vec3 UndeformedDirection(const vec3 undefPoint, const vec3 defDirection)
{
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	return normalize(invJacobian * defDirection);
}

vec3 SolveBS23(
	const vec3 undefPoint, 
	const vec3 defDirection, 
	const float maxRadius, 
	inout float distTraveled
)
{
	// Bogacki-Shampine ODE solver that returns the next undeformed point along the 
	// deformed ray path
	
	const float h = 0.01;
	const float c12 = 1. / 2.;
	const float c34 = 3. / 4.;
	const float c29 = 2. / 9.;
	const float c13 = 1. / 3.;
	const float c49 = 4. / 9.;
	const float c724 = 7. / 24.;
	const float c14 = 1. / 4.;
	const float c18 = 1. / 8.;
	const float paddedMaxRadius = maxRadius - h;
	
	vec3 y1 = vec3(0.);
	vec3 y2 = vec3(0.);
	vec3 y1_next = undefPoint;
	vec3 k1 = vec3(0.);
	vec3 k2 = vec3(0.);
	vec3 k3 = vec3(0.);
	vec3 k4 = vec3(0.);
	vec3 k1_next = UndeformedDirection(undefPoint, defDirection);
	
	float newDistTraveled = 0.;
	
	for(int i=0; i<16; i++)
	{	
		// these intermediate values are used to ensure correct values in y1-2 and k1-4 if the loop
		// runs out of iterations
		y1 = y1_next;
		k1 = k1_next;
		
		k2 = UndeformedDirection(y1 + (c12 * h) * k1, defDirection);
		k3 = UndeformedDirection(y1 + (c34 * h) * k2, defDirection);
		y2 = y1 + (c29 * h) * k1 + (c13 * h) * k2 + (c49 * h) * k3;
		k4 = UndeformedDirection(y2, defDirection);
		
		newDistTraveled += h;
		if(newDistTraveled > paddedMaxRadius)
		{
			break;
		}
		
		y1_next = y2;
		k1_next = k4;
	}
	
	distTraveled += newDistTraveled;
	
	return y1 + (c724 * h) * k1 + (c14 * h) * k2 + (c13 * h) * k3 + (c18 * h) * k4;
}

float OffsetError(const float distTraveled)
{
	const float permittedErrorPerLength = 0.001;
	return permittedErrorPerLength * distTraveled;
}

vec3 AdjustTerminationPoint(
	const vec3 undefPoint, 
	const vec3 undefDirection, 
	const float distTraveled
)
{
	float offset = 0.;
	
	for(int i=0; i<3; i++)
	{
		offset += 
			Sdf(undefPoint + undefDirection * offset) - 
			OffsetError(distTraveled + offset);
	}
	
	return undefPoint + undefDirection * offset;
}

vec3 NLST(
	const vec3 undefOrigin, 
	const vec3 defDirection, 
	const float distToOrigin, 
	const float undefPixelRadiusPerLength, 
	const float maxDist,
	const float maxRadius,
	inout bool hit
)
{
	// non-linear sphere tracing:
	// perform ODE traversal of the deformation field 
	// inside each unbounding-sphere using the inverse deformation jacobian
	// to transform the ray direction
	
	float distTraveled = distToOrigin;
	vec3 undefPoint = undefOrigin;
	hit = false;
	
	for(int i=0; i<64; i++)
	{
		float radius = Sdf(undefPoint);
		float minRadius = undefPixelRadiusPerLength * distTraveled;
		
		if(radius < minRadius)
		{
			hit = true;
			break;
		}
		
		if(distTraveled > maxDist || radius > maxRadius)
		{
			break;
		}
		
		if(radius < minRadius * 3.)
		{
			// use simple euler integration step when the radius is small
			undefPoint += UndeformedDirection(undefPoint, defDirection) * radius;
			distTraveled += radius;
		}
		else
		{
			// when the distance to the surface is great, utilize a
			// ODE solver to traverse the deformed path inside the unbounding-sphere
			undefPoint = SolveBS23(
				undefPoint, 
				defDirection,  
				radius, 
				distTraveled
			);
		}
	}
	
	if(hit)
	{
		undefPoint = AdjustTerminationPoint(
			undefPoint, 
			UndeformedDirection(undefPoint, defDirection), 
			distTraveled
		);
	}
	
	return undefPoint;
}

vec3 WorldSdfGradient(const vec3 undefPoint)
{
	const float step = 0.0001;
    const vec2 diff = vec2(1., -1.);
    vec3 undefGradient = 
		diff.xyy * Sdf(undefPoint + diff.xyy * step) + 
		diff.yyx * Sdf(undefPoint + diff.yyx * step) + 
		diff.yxy * Sdf(undefPoint + diff.yxy * step) + 
		diff.xxx * Sdf(undefPoint + diff.xxx * step);
					
	mat3 invJacobian = inverse(DeformationJacobian(undefPoint));
	vec3 defGradient = transpose(invJacobian) * undefGradient;
	
	return normalize(defGradient);
}

vec3 DeformationColor(const vec3 undefPoint)
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

vec3 JointWeightColor(const vec3 undefPoint)
{
	float weight = JointWeight(undefPoint, u_jointIndex);
	return vec3(weight, 0., 0.);
}

vec3 Shade(const vec3 undefHitPoint, const vec3 defDirection, const vec3 lightDir)
{
	vec3 normal = WorldSdfGradient(undefHitPoint);
	float diff = max(0., dot(normal, -lightDir));
	float spec = pow(max(0., dot(reflect(defDirection, normal), -lightDir)), 32.);
	
	vec3 ambientColor = vec3(0.1, 0.1, 0.2);
	vec3 lightColor = vec3(1., 0.9, 0.7);
	vec3 albedo = vec3(0.);
	
	if(u_jointIndex != -1)
	{
		albedo = JointWeightColor(undefHitPoint);
	}
	else
	{
		albedo = DeformationColor(undefHitPoint);
	}
	
	return albedo * (ambientColor + lightColor * (diff + spec));
}

float DeformedPointToDepth(const vec3 defPoint)
{
	vec4 clipPoint = u_VP * vec4(defPoint, 1.);
	return (clipPoint.z / clipPoint.w + 1.) * 0.5;
}

void main()
{
	if(u_renderMode == 0)
	{	
		// calculate the ray's origin and direction to the deformed start point
		vec3 undefOrigin = i_undeformedPos;
		vec3 defOrigin = Deform(undefOrigin);
		vec3 defDirection = defOrigin - u_cameraPos;
		float distToOrigin = length(defDirection);
		defDirection *= 1. / distToOrigin;
		
		// calculate how much the pixel radius scales with distance from the camera
		// by calculating the pixel's world-distance from the camera
		vec2 pixelUV = gl_FragCoord.xy / u_screenSize;
		vec4 pixelClipPos = vec4(pixelUV * 2. - 1., -1., 1.);
		vec4 pixelWorldPos = u_invVP * pixelClipPos;
		pixelWorldPos.xyz /= pixelWorldPos.w;
		float distToPixel = distance(pixelWorldPos.xyz, u_cameraPos);
		float pixelRadiusPerLength = u_pixelRadius / distToPixel;
		// to prepare the coefficient for use in undeformed space, we scale with the "deformed-to-undeformed volume scale factor" at the origin point,
		// that way we only have to multiply by distance traveled to get the undeformed cone radius at that point along the ray
		float undefPixelRadiusPerLength = pixelRadiusPerLength * determinant(inverse(DeformationJacobian(undefOrigin)));
		
		// define a max distance and radius in undeformed space
		float maxDist = distToOrigin + u_maxDistanceFromSurface;
		float maxRadius = u_maxRadius;
		
		bool hit = false;
		vec3 undefHitPoint = NLST(
			undefOrigin, 
			defDirection, 
			distToOrigin, 
			undefPixelRadiusPerLength,
			maxDist,
			maxRadius,
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