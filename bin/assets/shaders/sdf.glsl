
// source: https://iquilezles.org/articles/distfunctions/
float SmoothUnion(float d1, float d2, float k) 
{
    float h = clamp(0.5 + 0.5*(d2-d1)/k, 0., 1.);
    return mix(d2, d1, h) - k*h*(1.-h); 
}

float SD_Capsule(vec3 p, vec3 a, vec3 b, float radius)
{
	vec3 pa = p - a;
	vec3 ba = b - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
	return length(pa - ba * h) - radius;
}

float SD_Box(vec3 p, vec3 b)
{
	vec3 q = abs(p) - b;
	return length(max(q, 0.)) + min(max(q.x, max(q.y, q.z)), 0.);
}

// source: https://www.shadertoy.com/view/tsc3Rj & https://iquilezles.org/articles/mandelbulb/
float SD_Mandelbulb(vec3 p)
{
	float power = 8.;
	float dr = 1.0;
	float r = 0.0;
	vec3 z = p;
	
	for (int i=0; i<6; i++)
	{
		r = length(z);
		
		if (r > 2.){
			break;
		}
		
		// convert to polar coordinates
		float theta = acos(z.z / r);
		float phi = atan(z.y, z.x);

		dr =  pow(r, power - 1.) * power * dr + 1.;
		
		// scale and rotate the point
		float zr = pow(r, power);
		theta = theta * power;
		phi = phi * power;
		
		// convert back to cartesian coordinates
		z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
		z += p;
	}
	
	return 0.5 * log(r) * r / dr;
}

float Sdf(vec3 p)
{
	float torso = SD_Capsule(p, vec3(0., -0.5, 0.), vec3(0., 0.5, 0.), 0.25);
	vec3 mirroredP = vec3(abs(p.x), p.y, p.z);
	float arms = SD_Capsule(mirroredP, vec3(0.25, 0.5, 0.), vec3(1., 0.5, 0.), 0.1);
	float legs = SD_Capsule(mirroredP, vec3(0.25, -0.5, 0.), vec3(0.4, -1.4, 0.), 0.15);
	float head = length(p - vec3(0., 1.1, 0.)) - 0.2;
	float eyes = length(vec3(abs(p.x) - 0.1, p.y - 1.1, p.z + 0.2)) - 0.03;
	head = min(head, eyes);

	return SmoothUnion(SmoothUnion(min(torso, head), arms, 0.2), legs, 0.2);
}

/*float Sdf(vec3 p)
{
	return SD_Mandelbulb(p);
}*/