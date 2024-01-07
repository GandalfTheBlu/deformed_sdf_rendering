#pragma once
#include <GL/glew.h>
#include <vector>
#include <string>
#include <vec3.hpp>

namespace Engine
{
	struct VertexAttribute
	{
		GLuint location;
		GLuint components;
		GLint stride;
		GLint offset;
		GLenum type;

		void EnableAndConfigure() const;
		void Disable() const;
	};

	inline void VertexAttribute::EnableAndConfigure() const
	{
		glEnableVertexAttribArray(location);
		glVertexAttribPointer(location, components, type, GL_FALSE, stride, (const void*)offset);
	}

	inline void VertexAttribute::Disable() const
	{
		glDisableVertexAttribArray(location);
	}

	struct IndexAttribute
	{
		GLenum mode;
		GLuint offset;
		GLuint count;
		GLenum type;
	};

	struct DataBuffer
	{
		GLubyte* bufferStart;
		GLuint byteSize;
	};

	class RenderMesh final
	{
	private:
		GLuint vertexArrayObject;
		GLuint indexBufferObject;
		std::vector<GLuint> attributeBufferObjects;

		void Deinit();

	public:
		std::vector<IndexAttribute> primitiveGroups;

		RenderMesh();
		~RenderMesh();

		void Reload(
			const DataBuffer& indexBuffer, 
			const std::vector<IndexAttribute>& indexAttributes,
			const std::vector<DataBuffer>& vertexBuffers, 
			const std::vector<VertexAttribute>& vertexAttributes
		);

		void Bind() const;
		void Draw(GLuint primitiveGroupIndex) const;
		void Unbind() const;
	};
}