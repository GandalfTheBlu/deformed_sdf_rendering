#version 430
#include "assets/shaders/sdf.glsl"

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

uniform vec3 u_volumeMin;
uniform vec3 u_voxelSize;
uniform int u_voxelCountX;
uniform int u_voxelCountY;
uniform int u_voxelCountZ;

layout(std430, binding=0) buffer VoxelBuffer 
{
    float o_voxelData[];
};

void main() 
{
    ivec3 index3d = ivec3(gl_GlobalInvocationID);
    
	int voxelIndex = 
		index3d.x * u_voxelCountY * u_voxelCountZ + 
		index3d.y * u_voxelCountZ + index3d.z;

	vec3 voxelPos = u_volumeMin + u_voxelSize * vec3(index3d);

    o_voxelData[voxelIndex] = Sdf(voxelPos);
}