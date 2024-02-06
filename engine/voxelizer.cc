#include "voxelizer.h"

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
		const glm::vec3& voxelSize, std::vector<float>& outData,
		GLuint& outSizeX,
		GLuint& outSizeY,
		GLuint& outSizeZ
	)
	{
		glm::vec3 size = (volumeMax - volumeMin) / voxelSize + glm::vec3(1.f);
		assert(size.x >= 1.f && size.y >= 1.f && size.z >= 1.f);

		outSizeX = static_cast<GLuint>(size.x);
		outSizeY = static_cast<GLuint>(size.y);
		outSizeZ = static_cast<GLuint>(size.z);
		GLint linearSize = outSizeX * outSizeY * outSizeZ;
		outData.resize(linearSize, 0.f);

		// create buffer to write to and read from
		GLuint buffer = 0;
		glGenBuffers(1, &buffer);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, linearSize * sizeof(float), nullptr, GL_STATIC_COPY);

		// perform voxelization
		voxelizeShader.Use();
		voxelizeShader.SetVec3("u_volumeMin", &volumeMin[0]);
		voxelizeShader.SetVec3("u_volumeMax", &volumeMax[0]);
		voxelizeShader.SetVec3("u_voxelSize", &voxelSize[0]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
		glDispatchCompute(outSizeX, outSizeY, outSizeZ);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		voxelizeShader.StopUsing();

		// get memory from buffer
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, linearSize * sizeof(float), outData.data());

		// clean up
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glDeleteBuffers(1, &buffer);
	}
}