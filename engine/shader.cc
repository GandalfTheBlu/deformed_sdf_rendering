#include "shader.h"
#include "file_io.h"
#include <regex>
#include <memory>
#include <iostream>

namespace Engine
{
	Shader::Shader() :
		program(0)
	{}

	Shader::~Shader()
	{
		Deinit();
	}

	void FormatErrorLog(const char* message, std::string& outputStr)
	{
		std::string str = "\n";
		str.append(message);

		std::regex formatRgx("\\n(?:[0-9]+\\(([0-9]+)\\)\\s)(.+)");
		std::smatch formatMatch;
		std::string::const_iterator strStart(str.cbegin());
		while (std::regex_search(strStart, str.cend(), formatMatch, formatRgx))
		{
			std::string lineNumStr = formatMatch[1];
			std::string messageStr = formatMatch[2];
			outputStr += "\n\tline " + lineNumStr + messageStr;
			strStart = formatMatch.suffix().first;
		}
	}

	void Shader::Reload(const std::string& vertexFilePath, const std::string& fragmentFilePath)
	{
		std::string vertText;
		std::string fragText;
		ReadTextFile(vertexFilePath, vertText);
		ReadTextFile(fragmentFilePath, fragText);

		const char* vertText_C = vertText.c_str();
		const char* fragText_C = fragText.c_str();

		// create and compile vertex shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		GLint vertexShaderLength = static_cast<GLint>(vertText.size());
		glShaderSource(vertexShader, 1, &vertText_C, &vertexShaderLength);
		glCompileShader(vertexShader);

		GLint shaderLogSize;
		glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &shaderLogSize);
		if (shaderLogSize > 0)
		{
			std::unique_ptr<char[]> errorMessage(new char[shaderLogSize]);
			glGetShaderInfoLog(vertexShader, shaderLogSize, NULL, errorMessage.get());
			std::string errMsgStr;
			FormatErrorLog(errorMessage.get(), errMsgStr);
			std::cout << "[ERROR] failed to compile vertex shader '" << vertexFilePath << "': " << errMsgStr << std::endl;
			return;
		}

		// create and compile fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		GLint fragmentShaderLength = static_cast<GLint>(fragText.size());
		glShaderSource(fragmentShader, 1, &fragText_C, &fragmentShaderLength);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &shaderLogSize);
		if (shaderLogSize > 0)
		{
			std::unique_ptr<char[]> errorMessage(new char[shaderLogSize]);
			glGetShaderInfoLog(fragmentShader, shaderLogSize, NULL, errorMessage.get());
			std::string errMsgStr;
			FormatErrorLog(errorMessage.get(), errMsgStr);
			std::cout << "[ERROR] failed to compile fragment shader '" << fragmentFilePath << "': " << errMsgStr << std::endl;
			return;
		}

		// create and link program
		GLuint newProgram = glCreateProgram();
		glAttachShader(newProgram, vertexShader);
		glAttachShader(newProgram, fragmentShader);
		glLinkProgram(newProgram);

		glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &shaderLogSize);
		if (shaderLogSize > 0)
		{
			std::unique_ptr<char[]> errorMessage(new char[shaderLogSize]);
			glGetProgramInfoLog(newProgram, shaderLogSize, NULL, errorMessage.get());
			std::cout << "[ERROR] failed to link program '" << vertexFilePath << "' & '" << fragmentFilePath << "': " << errorMessage.get() << std::endl;
			return;
		}

		// clean up
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		if (program != 0)
			Deinit();

		program = newProgram;
	}

	void Shader::Deinit()
	{
		nameToLocation.clear();
		glDeleteProgram(program);
	}

	void Shader::Use()
	{
		glUseProgram(program);
	}

	void Shader::StopUsing()
	{
		glUseProgram(0);
	}

	void Shader::AddLocationIfNeeded(const std::string& name)
	{
		if (nameToLocation.find(name) == nameToLocation.end())
			nameToLocation[name] = glGetUniformLocation(program, name.c_str());
	}

	void Shader::SetFloat(const std::string& name, GLfloat value)
	{
		AddLocationIfNeeded(name);
		glUniform1f(nameToLocation[name], value);
	}

	void Shader::SetInt(const std::string& name, GLint value)
	{
		AddLocationIfNeeded(name);
		glUniform1i(nameToLocation[name], value);
	}

	void Shader::SetVec2(const std::string& name, const GLfloat* valuePtr, GLsizei count)
	{
		AddLocationIfNeeded(name);
		glUniform2fv(nameToLocation[name], count, valuePtr);
	}

	void Shader::SetVec3(const std::string& name, const GLfloat* valuePtr, GLsizei count)
	{
		AddLocationIfNeeded(name);
		glUniform3fv(nameToLocation[name], count, valuePtr);
	}

	void Shader::SetVec4(const std::string& name, const GLfloat* valuePtr, GLsizei count)
	{
		AddLocationIfNeeded(name);
		glUniform4fv(nameToLocation[name], count, valuePtr);
	}

	void Shader::SetMat3(const std::string& name, const GLfloat* valuePtr, GLsizei count)
	{
		AddLocationIfNeeded(name);
		glUniformMatrix3fv(this->nameToLocation[name], count, GL_FALSE, valuePtr);
	}

	void Shader::SetMat4(const std::string& name, const GLfloat* valuePtr, GLsizei count)
	{
		AddLocationIfNeeded(name);
		glUniformMatrix4fv(this->nameToLocation[name], count, GL_FALSE, valuePtr);
	}
}