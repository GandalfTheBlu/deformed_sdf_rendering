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

	void PreprocessShaderCode(const std::string& shaderCode, std::string& outShaderCode)
	{
		std::regex includeRgx("#include\\s\"([\\S\\s]+?)\"");
		std::smatch includeMatch;
		std::string::const_iterator strStart(shaderCode.cbegin());

		while (std::regex_search(strStart, shaderCode.cend(), includeMatch, includeRgx))
		{
			std::string includePath = includeMatch[1];
			std::string includeCode;
			ReadTextFile(includePath, includeCode);
			outShaderCode += includeMatch.prefix();
			outShaderCode += includeCode;
			strStart = includeMatch.suffix().first;
		}

		if (outShaderCode.size() == 0)
			outShaderCode = shaderCode;
		else
			outShaderCode += includeMatch.suffix();
	}

	bool TryLoadShader(const std::string& path, GLenum shaderType, GLuint& outShader)
	{
		std::string rawShaderText, shaderText;
		ReadTextFile(path, rawShaderText);
		PreprocessShaderCode(rawShaderText, shaderText);
		const char* shaderText_C = shaderText.c_str();

		GLuint shader = glCreateShader(shaderType);
		GLint shaderLength = static_cast<GLint>(shaderText.size());
		glShaderSource(shader, 1, &shaderText_C, &shaderLength);
		glCompileShader(shader);

		GLint shaderLogSize;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &shaderLogSize);
		if (shaderLogSize > 0)
		{
			std::unique_ptr<char[]> errorMessage(new char[shaderLogSize]);
			glGetShaderInfoLog(shader, shaderLogSize, NULL, errorMessage.get());
			std::string errMsgStr;
			FormatErrorLog(errorMessage.get(), errMsgStr);
			std::cout << "[ERROR] failed to compile shader '" << path << "': " << errMsgStr << std::endl;
			return false;
		}

		outShader = shader;
		return true;
	}

	struct TempShader
	{
		GLuint shader;

		~TempShader()
		{
			if (shader != 0)
				glDeleteShader(shader);
		}
	};

	bool Shader::Reload(
		const std::string& vertexFilePath, 
		const std::string& fragmentFilePath,
		const std::pair<std::string, std::string>& tesselationFilePaths
	)
	{
		// compile shaders
		TempShader vertexShader = { 0 };
		TempShader fragmentShader = { 0 };
		TempShader tessControlShader = { 0 };
		TempShader tessEvalShader = { 0 };
		bool useTess = false;

		if (!TryLoadShader(vertexFilePath, GL_VERTEX_SHADER, vertexShader.shader) ||
			!TryLoadShader(fragmentFilePath, GL_FRAGMENT_SHADER, fragmentShader.shader)) 
			return false;

		if (tesselationFilePaths.first.size() > 0 && tesselationFilePaths.second.size() > 0)
		{
			if (!TryLoadShader(tesselationFilePaths.first, GL_TESS_CONTROL_SHADER, tessControlShader.shader) ||
				!TryLoadShader(tesselationFilePaths.second, GL_TESS_EVALUATION_SHADER, tessEvalShader.shader))
				return false;

			useTess = true;
		}

		// create and link program
		GLuint newProgram = glCreateProgram();
		glAttachShader(newProgram, vertexShader.shader);
		glAttachShader(newProgram, fragmentShader.shader);
		if (useTess)
		{
			glAttachShader(newProgram, tessControlShader.shader);
			glAttachShader(newProgram, tessEvalShader.shader);
		}
		glLinkProgram(newProgram);

		GLint shaderLogSize;
		glGetProgramiv(newProgram, GL_INFO_LOG_LENGTH, &shaderLogSize);
		if (shaderLogSize > 0)
		{
			std::unique_ptr<char[]> errorMessage(new char[shaderLogSize]);
			glGetProgramInfoLog(newProgram, shaderLogSize, NULL, errorMessage.get());
			std::cout << "[ERROR] failed to link program '" << vertexFilePath << "' & '" << fragmentFilePath << "': " << errorMessage.get() << std::endl;
			glDeleteProgram(newProgram);
			return false;
		}

		if (program != 0)
			Deinit();

		program = newProgram;
		return true;
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