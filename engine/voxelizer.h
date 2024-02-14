#pragma once
#include "shader.h"
#include <vec3.hpp>
#include <vector>

namespace Engine
{
	class Voxelizer
	{
	private:
		Shader voxelizeShader;

	public:
		Voxelizer();

		bool Reload(const std::string& voxelizeShaderFilePath);
		// min and max inclusive (from edge to edge)
		void Voxelize(
			const glm::vec3& volumeMin,
			const glm::vec3& volumeMax,
			GLuint sizeX,
			GLuint sizeY,
			GLuint sizeZ,
			glm::vec3& outVoxelSize,
			std::vector<float>& outData
		);
	};
}