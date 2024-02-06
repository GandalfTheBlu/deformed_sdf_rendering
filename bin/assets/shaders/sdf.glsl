
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

float Sdf(vec3 p)
{
	float torso = SD_Capsule(p, vec3(0., -0.5, 0.), vec3(0., 0.5, 0.), 0.25);
	vec3 mirroredP = vec3(abs(p.x), p.y, p.z);
	float arms = SD_Capsule(mirroredP, vec3(0.25, 0.5, 0.), vec3(0.75, 0.5, 0.), 0.1);
	float legs = SD_Capsule(mirroredP, vec3(0.25, -0.5, 0.), vec3(0.25, -1.4, 0.), 0.15);

	return min(torso, min(arms, legs));
}