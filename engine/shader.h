#pragma once
#include <GL/glew.h>
#include <string>
#include <map>

namespace Engine
{
	class Shader final
	{
	private:
		GLuint program;
		std::map<std::string, GLint> nameToLocation;

		void AddLocationIfNeeded(const std::string& name);

		void Deinit();

	public:
		Shader();
		~Shader();
		
		bool Reload(
			const std::string& vertexFilePath, 
			const std::string& fragmentFilePath, 
			const std::pair<std::string, std::string>& tesselationFilePaths = {"", ""}
		);
		bool Reload(const std::string& computeFilePath);

		void Use();
		void StopUsing();

		void SetFloat(const std::string& name, GLfloat value);
		void SetInt(const std::string& name, GLint value);
		void SetFloats(const std::string& name, GLfloat* valuePtr, GLsizei count = 1);
		void SetInts(const std::string& name, GLint* valuePtr, GLsizei count = 1);
		void SetVec2(const std::string& name, const GLfloat* valuePtr, GLsizei count = 1);
		void SetVec3(const std::string& name, const GLfloat* valuePtr, GLsizei count = 1);
		void SetVec4(const std::string& name, const GLfloat* valuePtr, GLsizei count = 1);
		void SetMat3(const std::string& name, const GLfloat* valuePtr, GLsizei count = 1);
		void SetMat4(const std::string& name, const GLfloat* valuePtr, GLsizei count = 1);
	};
}