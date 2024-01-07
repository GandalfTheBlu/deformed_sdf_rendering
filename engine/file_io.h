#pragma once
#include <string>

namespace Engine
{
	void ReadTextFile(const std::string& path, std::string& text);
	void WriteTextFile(const std::string& path, const std::string& text, bool append);
}