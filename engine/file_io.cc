#include "file_io.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Engine
{
	bool ReadTextFile(const std::string& path, std::string& text)
	{
		std::ifstream file;
		file.open(path);

		if (!file.is_open())
		{
			std::cout << "[ERROR] failed to open file '" << path << "'" << std::endl;
			return false;
		}

		std::stringstream stringStream;
		stringStream << file.rdbuf();
		text = stringStream.str();

		file.close();
		return true;
	}

	bool WriteTextFile(const std::string& path, const std::string& text, bool append)
	{
		std::ofstream file;
		if (append)
			file.open(path, std::ios::app);
		else
			file.open(path);

		if (!file.is_open())
		{
			std::cout << "[ERROR] failed to open file '" << path << "'" << std::endl;
			return false;
		}

		file << text;

		file.close();
		return true;
	}

	bool ReadBinaryFile(const std::string& path, std::vector<char>& binary)
	{
		std::ifstream file;
		file.open(path, std::ios::binary);

		if (!file.is_open())
		{
			std::cout << "[ERROR] failed to open file '" << path << "'" << std::endl;
			return false;
		}

		file.seekg(0, std::ios::end);
		size_t fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		binary.resize(fileSize);
		file.read(binary.data(), fileSize);

		file.close();
		return true;
	}

	bool WriteBinaryFile(const std::string& path, const std::vector<char>& binary, bool append)
	{
		std::ofstream file;
		if (append)
			file.open(path, std::ios::app | std::ios::binary);
		else
			file.open(path, std::ios::binary);

		if (!file.is_open())
		{
			std::cout << "[ERROR] failed to open file '" << path << "'" << std::endl;
			return false;
		}

		file.write(binary.data(), binary.size());

		file.close();
		return true;
	}
}