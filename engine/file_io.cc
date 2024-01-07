#include "file_io.h"
#include <fstream>
#include <sstream>
#include <cassert>

namespace Engine
{
	void ReadTextFile(const std::string& path, std::string& text)
	{
		std::ifstream file;
		file.open(path);

		assert(file.is_open());

		std::stringstream stringStream;
		stringStream << file.rdbuf();
		text = stringStream.str();
		file.close();
	}

	void WriteTextFile(const std::string& path, const std::string& text, bool append)
	{
		std::ofstream file;
		if (append)
			file.open(path, std::ios::app);
		else
			file.open(path);

		file << text;

		file.close();
	}
}