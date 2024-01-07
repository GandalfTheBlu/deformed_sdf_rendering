#include "default_meshes.h"
#include <glm.hpp>

namespace Engine
{
	void GenerateUnitLine(RenderMesh& outMesh, MeshGeneratorSettings settings)
	{
		GLubyte indices[]
		{
			0, 1
		};

		GLfloat positions[]
		{
			0.f, 0.f, 0.f,
			0.f, 0.f, 1.f
		};

		DataBuffer indexBuffer;
		indexBuffer.bufferStart = indices;
		indexBuffer.byteSize = sizeof(indices);

		IndexAttribute indexAttrib;
		indexAttrib.mode = GL_LINES;
		indexAttrib.offset = 0;
		indexAttrib.count = 2;
		indexAttrib.type = GL_UNSIGNED_BYTE;

		DataBuffer posBuffer;
		posBuffer.bufferStart = (GLubyte*)positions;
		posBuffer.byteSize = sizeof(positions);

		VertexAttribute posAttrib;
		posAttrib.location = 0;
		posAttrib.components = 3;
		posAttrib.stride = sizeof(GLfloat) * 3;
		posAttrib.offset = 0;
		posAttrib.type = GL_FLOAT;

		outMesh.Reload(indexBuffer, { indexAttrib }, { posBuffer }, { posAttrib });
	}

	void GenerateUnitQuad(RenderMesh& outMesh, MeshGeneratorSettings settings)
	{
		GLubyte indices[]
		{
			0, 2, 1,
			0, 3, 2
		};

		GLfloat positions[]
		{
			-1.f, -1.f,
			-1.f, 1.f,
			1.f, 1.f,
			1.f, -1.f
		};

		GLfloat uvs[]
		{
			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f
		};

		GLfloat normals[]
		{
			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f
		};

		DataBuffer indexBuffer;
		indexBuffer.bufferStart = indices;
		indexBuffer.byteSize = sizeof(indices);

		IndexAttribute indexAttrib;
		indexAttrib.mode = GL_TRIANGLES;
		indexAttrib.offset = 0;
		indexAttrib.count = 6;
		indexAttrib.type = GL_UNSIGNED_BYTE;

		DataBuffer posBuffer;
		posBuffer.bufferStart = (GLubyte*)positions;
		posBuffer.byteSize = sizeof(positions);

		VertexAttribute posAttrib;
		posAttrib.location = 0;
		posAttrib.components = 2;
		posAttrib.stride = sizeof(GLfloat) * 2;
		posAttrib.offset = 0;
		posAttrib.type = GL_FLOAT;

		DataBuffer uvBuffer;
		uvBuffer.bufferStart = (GLubyte*)uvs;
		uvBuffer.byteSize = sizeof(uvs);

		VertexAttribute uvAttrib;
		uvAttrib.location = 1;
		uvAttrib.components = 2;
		uvAttrib.stride = sizeof(GLfloat) * 2;
		uvAttrib.offset = 0;
		uvAttrib.type = GL_FLOAT;

		DataBuffer normBuffer;
		normBuffer.bufferStart = (GLubyte*)normals;
		normBuffer.byteSize = sizeof(normals);

		VertexAttribute normAttrib;
		normAttrib.location = 2;
		normAttrib.components = 3;
		normAttrib.stride = sizeof(GLfloat) * 3;
		normAttrib.offset = 0;
		normAttrib.type = GL_FLOAT;

		std::vector<DataBuffer> vertexBuffers {posBuffer};
		std::vector<VertexAttribute> vertexAttributes {posAttrib};

		if (settings.value | MeshGeneratorSettings::E_Uvs)
		{
			vertexBuffers.push_back(uvBuffer);
			vertexAttributes.push_back(uvAttrib);
		}
		if (settings.value | MeshGeneratorSettings::E_Normals)
		{
			vertexBuffers.push_back(normBuffer);
			vertexAttributes.push_back(normAttrib);
		}

		outMesh.Reload(indexBuffer, { indexAttrib }, vertexBuffers, vertexAttributes);
	}

	void GenerateUnitCube(RenderMesh& outMesh, MeshGeneratorSettings settings)
	{
		GLubyte indices[]{
			// left
			0,3,1,
			3,2,1,
			// right
			4,7,5,
			7,6,5,
			// back
			8,11,9,
			11,10,9,
			// forward
			12,15,13,
			15,14,13,
			// down
			16,19,17,
			19,18,17,
			// up
			20,23,21,
			23,22,21
		};

		GLfloat positions[]{
			// left (0,1,2,3)
			-1.f, -1.f, 1.f,
			-1.f, 1.f, 1.f,
			-1.f, 1.f, -1.f,
			-1.f, -1.f, -1.f,
			// right (4,5,6,7)
			1.f, -1.f, -1.f,
			1.f, 1.f, -1.f,
			1.f, 1.f, 1.f,
			1.f, -1.f, 1.f,
			// back (8,9,10,11)
			-1.f, -1.f, -1.f,
			-1.f, 1.f, -1.f,
			1.f, 1.f, -1.f,
			1.f, -1.f, -1.f,
			// forward (12,13,14,15)
			1.f, -1.f, 1.f,
			1.f, 1.f, 1.f,
			-1.f, 1.f, 1.f,
			-1.f, -1.f, 1.f,
			// down (16,17,18,19)
			-1.f, -1.f, 1.f,
			-1.f, -1.f, -1.f,
			1.f, -1.f, -1.f,
			1.f, -1.f, 1.f,
			// up (20,21,22,23)
			-1.f, 1.f, -1.f,
			-1.f, 1.f, 1.f,
			1.f, 1.f, 1.f,
			1.f, 1.f, -1.f
		};

		GLfloat uvs[]
		{
			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f,

			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f,

			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f,

			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f,

			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f,

			0.f, 0.f,
			0.f, 1.f,
			1.f, 1.f,
			1.f, 0.f
		};

		GLfloat normals[]
		{
			-1.f, 0.f, 0.f,
			-1.f, 0.f, 0.f,
			-1.f, 0.f, 0.f,
			-1.f, 0.f, 0.f,

			1.f, 0.f, 0.f,
			1.f, 0.f, 0.f,
			1.f, 0.f, 0.f,
			1.f, 0.f, 0.f,

			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f,
			0.f, 0.f, -1.f,

			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,
			0.f, 0.f, 1.f,

			0.f, -1.f, 0.f,
			0.f, -1.f, 0.f,
			0.f, -1.f, 0.f,
			0.f, -1.f, 0.f,

			0.f, 1.f, 0.f,
			0.f, 1.f, 0.f,
			0.f, 1.f, 0.f,
			0.f, 1.f, 0.f
		};

		DataBuffer indexBuffer;
		indexBuffer.bufferStart = indices;
		indexBuffer.byteSize = sizeof(indices);

		IndexAttribute indexAttrib;
		indexAttrib.mode = GL_TRIANGLES;
		indexAttrib.offset = 0;
		indexAttrib.count = 36;
		indexAttrib.type = GL_UNSIGNED_BYTE;

		DataBuffer posBuffer;
		posBuffer.bufferStart = (GLubyte*)positions;
		posBuffer.byteSize = sizeof(positions);

		VertexAttribute posAttrib;
		posAttrib.location = 0;
		posAttrib.components = 3;
		posAttrib.stride = sizeof(GLfloat) * 3;
		posAttrib.offset = 0;
		posAttrib.type = GL_FLOAT;

		DataBuffer uvBuffer;
		uvBuffer.bufferStart = (GLubyte*)uvs;
		uvBuffer.byteSize = sizeof(uvs);

		VertexAttribute uvAttrib;
		uvAttrib.location = 1;
		uvAttrib.components = 2;
		uvAttrib.stride = sizeof(GLfloat) * 2;
		uvAttrib.offset = 0;
		uvAttrib.type = GL_FLOAT;

		DataBuffer normBuffer;
		normBuffer.bufferStart = (GLubyte*)normals;
		normBuffer.byteSize = sizeof(normals);

		VertexAttribute normAttrib;
		normAttrib.location = 2;
		normAttrib.components = 3;
		normAttrib.stride = sizeof(GLfloat) * 3;
		normAttrib.offset = 0;
		normAttrib.type = GL_FLOAT;

		std::vector<DataBuffer> vertexBuffers{ posBuffer };
		std::vector<VertexAttribute> vertexAttributes{ posAttrib };

		if (settings.value | MeshGeneratorSettings::E_Uvs)
		{
			vertexBuffers.push_back(uvBuffer);
			vertexAttributes.push_back(uvAttrib);
		}
		if (settings.value | MeshGeneratorSettings::E_Normals)
		{
			vertexBuffers.push_back(normBuffer);
			vertexAttributes.push_back(normAttrib);
		}

		outMesh.Reload(indexBuffer, { indexAttrib }, vertexBuffers, vertexAttributes);
	}

	void GenerateUnitSphere(RenderMesh& outMesh, MeshGeneratorSettings settings)
	{
		const int resX = 10;
		const int resY = 10;
		const int vertexCount = 6 * (resX + 1) * (resY + 1);
		const int indexCount = 36 * resX * resY;
		const int positionCount = vertexCount * 3;

		std::vector<GLuint> indices;
		std::vector<GLfloat> positions;
		std::vector<GLfloat> uvs;
		std::vector<GLfloat> normals;

		indices.reserve(indexCount);
		positions.reserve(positionCount);

		// face order: left, right, back, forward, down, up
		glm::vec3 faceCorners[6]{
			glm::vec3(-1.f, -1.f, 1.f),
			glm::vec3(1.f, -1.f, -1.f),
			glm::vec3(-1.f, -1.f, -1.f),
			glm::vec3(1.f, -1.f, 1.f),
			glm::vec3(-1.f, -1.f, 1.f),
			glm::vec3(-1.f, 1.f, -1.f)
		};

		// right and up vector for each face
		glm::vec3 faceAxis[12]{
			glm::vec3(0.f, 0.f, -1.f),
			glm::vec3(0.f, 1.f, 0.f),

			glm::vec3(0.f, 0.f, 1.f),
			glm::vec3(0.f, 1.f, 0.f),

			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(0.f, 1.f, 0.f),

			glm::vec3(-1.f, 0.f, 0.f),
			glm::vec3(0.f, 1.f, 0.f),

			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(0.f, 0.f, -1.f),

			glm::vec3(1.f, 0.f, 0.f),
			glm::vec3(0.f, 0.f, 1.f)
		};

		// construct faces
		for (int n = 0; n < 6; n++)
		{
			// create grid
			int faceStartIndex = n * (resX + 1) * (resY + 1);
			for (int i = 0; i < resX + 1; i++)
			{
				for (int j = 0; j < resY + 1; j++)
				{
					GLfloat u = (GLfloat)i / resX;
					GLfloat v = (GLfloat)j / resY;
					glm::vec3 gridPoint = faceCorners[n] + faceAxis[2 * n] * (2.f * u) + faceAxis[2 * n + 1] * (2.f * v);
					glm::vec3 vertex = glm::normalize(gridPoint);

					positions.push_back(vertex.x);
					positions.push_back(vertex.y);
					positions.push_back(vertex.z);

					uvs.push_back(u);
					uvs.push_back(v);

					normals.push_back(vertex.x);
					normals.push_back(vertex.y);
					normals.push_back(vertex.z);

					// create indicis on the fly (not if on right- or top edge)
					if (i >= resX || j >= resY)
						continue;

					GLuint i00 = faceStartIndex + i * (resY + 1) + j;
					GLuint i01 = faceStartIndex + i * (resY + 1) + j + 1;
					GLuint i11 = faceStartIndex + (i + 1) * (resY + 1) + j + 1;
					GLuint i10 = faceStartIndex + (i + 1) * (resY + 1) + j;

					indices.push_back(i10);
					indices.push_back(i01);
					indices.push_back(i00);

					indices.push_back(i11);
					indices.push_back(i01);
					indices.push_back(i10);
				}
			}
		}

		DataBuffer indexBuffer;
		indexBuffer.bufferStart = (GLubyte*)indices.data();
		indexBuffer.byteSize = sizeof(GLuint) * indices.size();

		IndexAttribute indexAttrib;
		indexAttrib.mode = GL_TRIANGLES;
		indexAttrib.offset = 0;
		indexAttrib.count = indices.size();
		indexAttrib.type = GL_UNSIGNED_INT;

		DataBuffer posBuffer;
		posBuffer.bufferStart = (GLubyte*)positions.data();
		posBuffer.byteSize = sizeof(GLfloat) * positions.size();

		VertexAttribute posAttrib;
		posAttrib.location = 0;
		posAttrib.components = 3;
		posAttrib.stride = sizeof(GLfloat) * 3;
		posAttrib.offset = 0;
		posAttrib.type = GL_FLOAT;

		DataBuffer uvBuffer;
		uvBuffer.bufferStart = (GLubyte*)uvs.data();
		uvBuffer.byteSize = sizeof(GLfloat) * uvs.size();

		VertexAttribute uvAttrib;
		uvAttrib.location = 1;
		uvAttrib.components = 2;
		uvAttrib.stride = sizeof(GLfloat) * 2;
		uvAttrib.offset = 0;
		uvAttrib.type = GL_FLOAT;

		DataBuffer normBuffer;
		normBuffer.bufferStart = (GLubyte*)normals.data();
		normBuffer.byteSize = sizeof(GLfloat) * normals.size();

		VertexAttribute normAttrib;
		normAttrib.location = 2;
		normAttrib.components = 3;
		normAttrib.stride = sizeof(GLfloat) * 3;
		normAttrib.offset = 0;
		normAttrib.type = GL_FLOAT;

		std::vector<DataBuffer> vertexBuffers{ posBuffer };
		std::vector<VertexAttribute> vertexAttributes{ posAttrib };

		if (settings.value | MeshGeneratorSettings::E_Uvs)
		{
			vertexBuffers.push_back(uvBuffer);
			vertexAttributes.push_back(uvAttrib);
		}
		if (settings.value | MeshGeneratorSettings::E_Normals)
		{
			vertexBuffers.push_back(normBuffer);
			vertexAttributes.push_back(normAttrib);
		}

		outMesh.Reload(indexBuffer, { indexAttrib }, vertexBuffers, vertexAttributes);
	}

	void GenerateUnitCapsule(RenderMesh& outMesh, MeshGeneratorSettings settings)
	{
		const int res = 20;
		const int resCaps = 6;

		std::vector<GLuint> indices;
		std::vector<GLfloat> positions;
		std::vector<GLfloat> uvs;
		std::vector<GLfloat> normals;

		for (int i = 0; i <= res; i++)
		{
			GLfloat u = (GLfloat)i / res;
			GLfloat angle = 2.f * 3.141592f * u;
			GLfloat x = glm::cos(angle);
			GLfloat z = glm::sin(angle);

			GLuint i0 = positions.size() / 3;

			positions.push_back(x);
			positions.push_back(-1.f);
			positions.push_back(z);

			positions.push_back(x);
			positions.push_back(1.f);
			positions.push_back(z);

			uvs.push_back(u);
			uvs.push_back(0.f);

			uvs.push_back(u);
			uvs.push_back(1.f);

			normals.push_back(x);
			normals.push_back(0.f);
			normals.push_back(z);

			normals.push_back(x);
			normals.push_back(0.f);
			normals.push_back(z);

			if (i < res)
			{
				indices.push_back(i0);
				indices.push_back(i0 + 3);
				indices.push_back(i0 + 1);

				indices.push_back(i0);
				indices.push_back(i0 + 2);
				indices.push_back(i0 + 3);
			}
		}

		GLfloat dir = 1.f;
		const int nextLayerOffset = res + 1;

		for (int n = 0; n < 2; n++)
		{
			for (int j = 0; j <= resCaps; j++)
			{
				for (int i = 0; i <= res; i++)
				{
					GLfloat u = (GLfloat)i / res;
					GLfloat v = (GLfloat)j / resCaps;
					GLfloat vertAngle = 0.5f * 3.141592f * v;
					GLfloat yAngle = 2.f * 3.141592f * u;
					GLfloat c = glm::cos(vertAngle);
					GLfloat x = glm::cos(yAngle) * c;
					GLfloat y = glm::sin(vertAngle);
					GLfloat z = glm::sin(yAngle) * c;

					GLuint i0 = positions.size() / 3;

					positions.push_back(x);
					positions.push_back(dir * y + dir);
					positions.push_back(z);

					uvs.push_back(u);
					uvs.push_back(v);

					normals.push_back(x);
					normals.push_back(dir * y);
					normals.push_back(z);

					if (i < res && j < resCaps)
					{
						if (dir == 1.f)
						{
							indices.push_back(i0);
							indices.push_back(i0 + 1);
							indices.push_back(i0 + nextLayerOffset + 1);

							if (j < resCaps - 1)
							{
								indices.push_back(i0);
								indices.push_back(i0 + nextLayerOffset + 1);
								indices.push_back(i0 + nextLayerOffset);
							}
						}
						else
						{
							indices.push_back(i0);
							indices.push_back(i0 + nextLayerOffset + 1);
							indices.push_back(i0 + 1);

							if (j < resCaps - 1)
							{
								indices.push_back(i0);
								indices.push_back(i0 + nextLayerOffset);
								indices.push_back(i0 + nextLayerOffset + 1);
							}
						}
					}
				}
			}

			dir = -1.f;
		}

		DataBuffer indexBuffer;
		indexBuffer.bufferStart = (GLubyte*)indices.data();
		indexBuffer.byteSize = sizeof(GLuint) * indices.size();

		IndexAttribute indexAttrib;
		indexAttrib.mode = GL_TRIANGLES;
		indexAttrib.offset = 0;
		indexAttrib.count = indices.size();
		indexAttrib.type = GL_UNSIGNED_INT;

		DataBuffer posBuffer;
		posBuffer.bufferStart = (GLubyte*)positions.data();
		posBuffer.byteSize = sizeof(GLfloat) * positions.size();

		VertexAttribute posAttrib;
		posAttrib.location = 0;
		posAttrib.components = 3;
		posAttrib.stride = sizeof(GLfloat) * 3;
		posAttrib.offset = 0;
		posAttrib.type = GL_FLOAT;

		DataBuffer uvBuffer;
		uvBuffer.bufferStart = (GLubyte*)uvs.data();
		uvBuffer.byteSize = sizeof(GLfloat) * uvs.size();

		VertexAttribute uvAttrib;
		uvAttrib.location = 1;
		uvAttrib.components = 2;
		uvAttrib.stride = sizeof(GLfloat) * 2;
		uvAttrib.offset = 0;
		uvAttrib.type = GL_FLOAT;

		DataBuffer normBuffer;
		normBuffer.bufferStart = (GLubyte*)normals.data();
		normBuffer.byteSize = sizeof(GLfloat) * normals.size();

		VertexAttribute normAttrib;
		normAttrib.location = 2;
		normAttrib.components = 3;
		normAttrib.stride = sizeof(GLfloat) * 3;
		normAttrib.offset = 0;
		normAttrib.type = GL_FLOAT;

		std::vector<DataBuffer> vertexBuffers{ posBuffer };
		std::vector<VertexAttribute> vertexAttributes{ posAttrib };

		if (settings.value | MeshGeneratorSettings::E_Uvs)
		{
			vertexBuffers.push_back(uvBuffer);
			vertexAttributes.push_back(uvAttrib);
		}
		if (settings.value | MeshGeneratorSettings::E_Normals)
		{
			vertexBuffers.push_back(normBuffer);
			vertexAttributes.push_back(normAttrib);
		}

		outMesh.Reload(indexBuffer, { indexAttrib }, vertexBuffers, vertexAttributes);
	}
}