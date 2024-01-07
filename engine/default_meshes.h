#pragma once
#include "render_mesh.h"

namespace Engine
{
	struct MeshGeneratorSettings
	{
		enum : int
		{
			E_Default = 0,
			E_Normals = 1,
			E_Uvs = 2
		};

		int value;
	};

	void GenerateUnitLine(RenderMesh& outMesh);
	void GenerateUnitQuad(RenderMesh& outMesh, MeshGeneratorSettings settings = { MeshGeneratorSettings::E_Default });
	void GenerateUnitCube(RenderMesh& outMesh, MeshGeneratorSettings settings = { MeshGeneratorSettings::E_Default });
	void GenerateUnitSphere(RenderMesh& outMesh, MeshGeneratorSettings settings = { MeshGeneratorSettings::E_Default });
	void GenerateUnitCapsule(RenderMesh& outMesh, MeshGeneratorSettings settings = { MeshGeneratorSettings::E_Default });
}