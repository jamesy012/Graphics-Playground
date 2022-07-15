#pragma once
//https://gist.github.com/tilkinsc/13191c0c1e5d6b25fbe79bbd2288a673

#include <vulkan/vulkan.h>

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
	VkFormat SelectVKFormatFromDXGI(uint32_t aFormat);
};
struct DDSFileHeader {
	union {
		uint8_t ddsCode[4]; // 'DDS
		uint32_t ddsCodeValue;
	};
	uint32_t size; //size of header always 124
	uint32_t flags; //. bitflags that tells you if data is present in the file
		//      CAPS         0x1
		//      HEIGHT       0x2
		//      WIDTH        0x4
		//      PITCH        0x8
		//      PIXELFORMAT  0x1000
		//      MIPMAPCOUNT  0x20000
		//      LINEARSIZE   0x80000
		//      DEPTH        0x800000
	uint32_t height;
	uint32_t width;
	uint32_t pitchOrLinearSize; //. bytes per scan line in an uncompressed texture, or bytes in the top level texture for a compressed texture
		//     D3DX11.lib and other similar libraries unreliably or inconsistently provide the pitch, convert with
		//     DX* && BC*: max( 1, ((width+3)/4) ) * block-size
		//     *8*8_*8*8 && UYVY && YUY2: ((width+1) >> 1) * 4
		//     (width * bits-per-pixel + 7)/8 (divide by 8 for byte alignment, whatever that means)
	uint32_t depth; //depth of volume texture in pixels
	uint32_t mipMapCount;
	uint32_t unused[11];
	uint32_t size2; //always 32
	uint32_t dataFlags; //. bitflags that tells you if data is present in the file for following 28 bytes
	//      ALPHAPIXELS  0x1
	//      ALPHA        0x2
	//      FOURCC       0x4
	//      RGB          0x40
	//      YUV          0x200
	//      LUMINANCE    0x20000
	union {
		uint8_t fourCC[4]; // fileFormat
		uint32_t fourCCValue;
	};
	uint32_t RGBBitCount; // bits per pixel
	uint32_t RBitMask;
	uint32_t GBitMask;
	uint32_t BBitMask;
	uint32_t ABitMask;
	uint32_t caps; //. 0x1000 for a texture w/o mipmaps
		//      0x401008 for a texture w/ mipmaps
		//      0x1008 for a cube map
	uint32_t caps2; //. bitflags that tells you if data is present in the file
		//      CUBEMAP           0x200     Required for a cube map.
		//      CUBEMAP_POSITIVEX 0x400     Required when these surfaces are stored in a cube map.
		//      CUBEMAP_NEGATIVEX 0x800     ^
		//      CUBEMAP_POSITIVEY 0x1000    ^
		//      CUBEMAP_NEGATIVEY 0x2000    ^
		//      CUBEMAP_POSITIVEZ 0x4000    ^
		//      CUBEMAP_NEGATIVEZ 0x8000    ^
		//      VOLUME            0x200000  Required for a volume texture.
	uint32_t caps3; //unused
	uint32_t caps4; //unused
	uint32_t reserved2; //unused
};

#define MakeFourCC(x, y, z, w) (x | y << 8 | z << 16 | w << 24);
static constexpr uint32_t DDSMagic = MakeFourCC('D', 'D', 'S', ' ');
static constexpr uint32_t DXT10Magic = MakeFourCC('D', 'X', '1', '0');
static constexpr uint32_t ATI2Magic = MakeFourCC('A', 'T', 'I', '2');

struct DDSDXT10Header {
	uint32_t dxgiFormat;
	uint32_t resourceDimension;
	uint32_t miscFlag;
	uint32_t arraySize;
	uint32_t miscFlags2;
};

Job::Work DDSLoader::GetWork(FileIO::Path aPath) {
	FileIO::File file = FileIO::LoadFile(aPath);
	char* data = file.mData;
	DDSFileHeader* header = (DDSFileHeader*)data;
	uint32_t pixelDataOffset = header->size + 4;
	DDSDXT10Header* dxt10 = (DDSDXT10Header*)(data + pixelDataOffset);

	ASSERT(data[0] == 'D' && data[1] == 'D' && data[2] == 'S');
	ASSERT(header->ddsCodeValue == DDSMagic);

	VkFormat desiredFormat = VK_FORMAT_UNDEFINED;

	if(header->fourCC[0] == 'D') {
		switch(header->fourCCValue) {
			case DXT10Magic: //DX10
				desiredFormat = SelectVKFormatFromDXGI(dxt10->dxgiFormat);
				ASSERT(dxt10->resourceDimension == 3); //TEXTURE2D
				pixelDataOffset += sizeof(DDSDXT10Header);
				break;
			default:
				break;
		}
	}
	//ATI2 - BC5 https://en.wikipedia.org/wiki/3Dc
	if(header->fourCC[0] == 'A') {
		switch(header->fourCCValue) {
			case ATI2Magic:
				desiredFormat = VK_FORMAT_BC5_UNORM_BLOCK;
				//no blue channel
				//possibly r and b are flipped?
				//should load this, then do a conversion to another format..
				//probably compute shader that adds in the blue channel?
				LOGGER::Log("ATI2 texture is incorrect - investigte\n");
				break;
			default:
				break;
		}
	}
	ASSERT(desiredFormat != VK_FORMAT_UNDEFINED);

	ASSERT(header->flags & 0x2); //height
	ASSERT(header->flags & 0x4); //width
	ASSERT(header->dataFlags & 0x4); //fourCC

	uint8_t mipCount = 1;
	if(header->flags & 0x20000) { //mipcount
		mipCount = header->mipMapCount;
	}

	//todo mips

	mImage->CreateFromData(data + pixelDataOffset, desiredFormat, {header->width, header->height}, aPath.String().c_str());

	FileIO::UnloadFile(file);

	return Job::Work();
}

VkFormat DDSLoader::SelectVKFormatFromDXGI(uint32_t aFormat) {
	switch(aFormat) {
		case 98: //DXGI_FORMAT_BC7_UNORM
			return VK_FORMAT_BC7_UNORM_BLOCK;
	}
	return VK_FORMAT_UNDEFINED;
}
