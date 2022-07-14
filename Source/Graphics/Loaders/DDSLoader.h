#pragma once

#include "LoaderBase.h"
#include "Graphics/Image.h"

#include "PlatformDebug.h"

class DDSLoader : public LoaderBase {
private:
	struct AsyncLoadData {
		//
	};

public:
	virtual Job::Work GetWork(FileIO::Path aPath) override;

private:
};

Job::Work DDSLoader::GetWork(FileIO::Path aPath) {
	//todo
    FileIO::File file = FileIO::LoadFile(aPath);
    char* data = file.mData;
    ASSERT(data[0] == 'D' && data[1] == 'D' && data[2] == 'S');
	ASSERT(false);
}
