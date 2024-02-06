#version 430
#include "assets/shaders/sdf.glsl"

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

uniform vec3 u_volumeMin;
uniform vec3 u_volumeMax;
uniform vec3 u_voxelSize;

layout(std430, binding=0) buffer VoxelBuffer 
{
    float o_voxelData[];
};

void main() 
{
    ivec3 index3d = ivec3(gl_GlobalInvocationID);
	ivec3 volumeSize = ivec3((u_volumeMax - u_volumeMin) / u_voxelSize) + ivec3(1);
    
	int voxelIndex = 
		index3d.x * volumeSize.y * volumeSize.z + 
		index3d.y * volumeSize.z + index3d.z;

	vec3 voxelPos = u_volumeMin + u_voxelSize * vec3(index3d);

    o_voxelData[voxelIndex] = Sdf(voxelPos);
}