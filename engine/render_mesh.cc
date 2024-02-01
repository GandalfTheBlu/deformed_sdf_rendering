#include "render_mesh.h"

namespace Engine
{
	RenderMesh::RenderMesh() :
		vertexArrayObject(0),
		indexBufferObject(0)
	{}

	RenderMesh::~RenderMesh()
	{
		Deinit();
	}

	void RenderMesh::Deinit()
	{
		glDeleteVertexArrays(1, &vertexArrayObject);
		glDeleteBuffers(1, &indexBufferObject);

		if (attributeBufferObjects.size() > 0)
			glDeleteBuffers(attributeBufferObjects.size(), &attributeBufferObjects[0]);

		vertexArrayObject = 0;
		indexBufferObject = 0;
		attributeBufferObjects.clear();
		primitiveGroups.clear();
	}

	void RenderMesh::Reload(
		const DataBuffer& indexBuffer,
		const std::vector<IndexAttribute>& indexAttributes,
		const std::vector<DataBuffer>& vertexBuffers,
		const std::vector<VertexAttribute>& vertexAttributes)
	{
		if (vertexArrayObject != 0)
			Deinit();

		primitiveGroups = indexAttributes;

		// start setup of vertex array object
		glGenVertexArrays(1, &vertexArrayObject);
		glBindVertexArray(vertexArrayObject);

		// store the index data in the index buffer object
		glGenBuffers(1, &indexBufferObject);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->indexBufferObject);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.byteSize, indexBuffer.bufferStart, GL_STATIC_DRAW);

		for (size_t i = 0; i < vertexBuffers.size(); i++) 
		{
			GLuint attributeBuffer = 0;
			glGenBuffers(1, &attributeBuffer);
			attributeBufferObjects.push_back(attributeBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, attributeBuffer);
			glBufferData(GL_ARRAY_BUFFER, vertexBuffers[i].byteSize, vertexBuffers[i].bufferStart, GL_STATIC_DRAW);

			// the vertex data is either packed into one buffer or separated into individual buffers
			if (vertexBuffers.size() == 1) 
			{
				for (size_t j = 0; j < vertexAttributes.size(); j++) 
				{
					vertexAttributes[j].EnableAndConfigure();
				}
			}
			else 
			{
				vertexAttributes[i].EnableAndConfigure();
			}
		}

		// end setup of vertex array object
		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		for (size_t i = 0; i < vertexAttributes.size(); i++) 
			vertexAttributes[i].Disable();
	}

	void RenderMesh::Bind() const
	{
		glBindVertexArray(vertexArrayObject);
	}

	void RenderMesh::Draw(GLuint primitiveGroupIndex, GLenum mode) const
	{
		IndexAttribute group = this->primitiveGroups[primitiveGroupIndex];

		glDrawElements(mode, group.count, group.type, (void*)group.offset);
	}

	void RenderMesh::Unbind() const
	{
		glBindVertexArray(0);
	}
}