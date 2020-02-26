#pragma once

#include <unirender/Texture.h>

namespace model
{

class TextureLoader
{
public:
	static ur::TexturePtr LoadFromFile(const char* filepath, int mipmap_levels = 0);
	static ur::TexturePtr LoadFromMemory(const unsigned char* pixels, int width, int height, int channels);

}; // TextureLoader

}