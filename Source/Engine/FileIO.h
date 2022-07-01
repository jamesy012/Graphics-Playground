#pragma once

#include <filesystem>

namespace FileIO {
	struct Path {
		Path() : mPath("") {}
		Path(std::string aPath) : mPath(aPath) {}
		Path(const char* aPath) : mPath(aPath) {}
		operator std::string() {
			return mPath;
		}

		std::string String() const {
			return mPath.generic_string();
		}

		std::string Extension() const {
			return mPath.extension().generic_string();
		}

	private:
		const std::filesystem::path mPath;
	};

	struct File {

		Path mPath;
		char* mData	 = nullptr;
		size_t mSize = 0;
	};
	File LoadFile(Path aPath);
	void UnloadFile(File aFile);
}; // namespace FileIO