#pragma once
#include "render_mesh.h"
#include <vec3.hpp>

namespace Engine
{
	typedef float(*ScalarField_t)(const glm::vec3&);

	void TriangulateScalarField(
		RenderMesh& outMesh, 
		ScalarField_t field, 
		const glm::vec3& min, 
		const glm::vec3& max, 
		float cellSize,
		float surfaceOffset
	);
}