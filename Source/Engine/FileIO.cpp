#include "FileIO.h"

#include <fstream>
#include <sstream>
#include <filesystem>

#include "PlatformDebug.h"

namespace FileIO {

    File LoadFile(Path aPath) {
        LOG::Log("Current Filepath: (%s)/%s\n", std::filesystem::current_path().c_str(), aPath.mPath.c_str());

        std::ifstream fileStream(aPath);
        std::stringstream dataStream;
        dataStream << fileStream.rdbuf();

        File file;
        file.mData = dataStream.str();
        file.mSize = file.mData.size();
        //file.mPath = aPath;

        return file;
    }

    const uint32_t* File::GetAsUInt32() {
        return (uint32_t*)mData.data();
    }

};