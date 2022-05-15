#pragma once

#include <string>

namespace FileIO {
    struct Path {
        Path() : mPath("") {}
        Path(std::string aPath) : mPath(aPath) {}
        Path(const char* aPath) : mPath(aPath) {}
        operator std::string() {return mPath;}

        const std::string mPath;
    };

    struct File {

        Path mPath;
        char* mData = nullptr;
        size_t mSize = 0;
    };
    File LoadFile(Path aPath);
    void UnloadFile(File aFile);
};