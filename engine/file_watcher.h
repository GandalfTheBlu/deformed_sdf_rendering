#pragma once
#include <string>

namespace Engine
{
	class FileWatcher
	{
	private:
		double lastChangeTime;

	public:
		std::string filePath;

		FileWatcher();

		void Init(const std::string& _filePath);
		bool NewVersionAvailable();
	};
}