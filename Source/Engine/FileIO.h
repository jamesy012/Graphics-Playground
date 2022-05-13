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
        std::string mData;
        size_t mSize;

        const uint32_t* GetAsUInt32();
    };
    File LoadFile(Path aPath);
};