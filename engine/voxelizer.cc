#include "voxelizer.h"
#include <glm.hpp>

namespace Engine
{
	Voxelizer::Voxelizer()
	{}

	bool Voxelizer::Reload(const std::string& voxelizeShaderFilePath)
	{
		return voxelizeShader.Reload(voxelizeShaderFilePath);
	}

	void Voxelizer::Voxelize(
		const glm::vec3& volumeMin, 
		const glm::vec3& volumeMax, 
		GLuint sizeX,
		GLuint sizeY,
		GLuint sizeZ,
		glm::vec3& outVoxelSize,
		std::vector<float>& outData
	)
	{
		GLint linearSize = sizeX * sizeY * sizeZ;
		outData.resize(linearSize, 0.f);

		outVoxelSize = (volumeMax - volumeMin) / glm::vec3(sizeX, sizeY, sizeZ);

		// create buffer to write to and read from
		GLuint buffer = 0;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, linearSize * sizeof(float), nullptr, GL_STATIC_COPY);

		// perform voxelization
		voxelizeShader.Use();
		voxelizeShader.SetVec3("u_volumeMin", &volumeMin[0]);
		voxelizeShader.SetVec3("u_voxelSize", &outVoxelSize[0]);
		voxelizeShader.SetInt("u_voxelCountX", (GLint)sizeX);
		voxelizeShader.SetInt("u_voxelCountY", (GLint)sizeY);
		voxelizeShader.SetInt("u_voxelCountZ", (GLint)sizeZ);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glDispatchCompute(sizeX, sizeY, sizeZ);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		voxelizeShader.StopUsing();

		// get memory from buffer
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, linearSize * sizeof(float), outData.data());

		// clean up
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &buffer);
	}
}