#include "FileIO.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#include "PlatformDebug.h"

namespace FileIO {

	File LoadFile(Path aPath) {
		LOG::Log("Current Filepath: (%s)/%s\n", std::filesystem::current_path().string().c_str(), aPath.mPath.c_str());
		LOG::Log("loading: %s\n", std::filesystem::absolute(aPath.mPath.c_str()).string().c_str());

		File file;
		std::ifstream fileStream(aPath, std::ios::binary | std::ios::in | std::ios::ate);
		if(fileStream.is_open()) {
			file.mSize = fileStream.tellg();
			fileStream.seekg(0, std::ios::beg);
			file.mData = new char[file.mSize];
			fileStream.read(file.mData, file.mSize);
			fileStream.close();
		} else {
			ASSERT(false);
		}

		return file;
	}

	void UnloadFile(File& aFile) {
		if(aFile.mData) {
			delete[] aFile.mData;
			aFile.mData = nullptr;
		}
	}

}; // namespace FileIO