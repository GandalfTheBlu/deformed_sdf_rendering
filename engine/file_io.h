#pragma once
#include <string>
#include <vector>

namespace Engine
{
	bool CreateDirectory(const std::string& path);
	bool ReadTextFile(const std::string& path, std::string& text);
	bool WriteTextFile(const std::string& path, const std::string& text, bool append);
	bool ReadBinaryFile(const std::string& path, std::vector<char>& binary);
	bool WriteBinaryFile(const std::string& path, const std::vector<char>& binary, bool append);
}