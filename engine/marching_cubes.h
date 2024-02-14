#pragma once
#include "render_mesh.h"
#include <vec3.hpp>

namespace Engine
{
	void TriangulateScalarField(
		const float* p_scalarField, 
		size_t sizeX,
		size_t sizeY,
		size_t sizeZ,
		const glm::vec3& volumeMin,
		const glm::vec3& cellSize,
		float surfaceOffset,
		glm::vec3& outMinCorner,
		glm::vec3& outMaxCorner,
		RenderMesh& outMesh 
	);
}