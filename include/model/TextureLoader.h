#pragma once

#include <unirender/typedef.h>

namespace ur { class Device; }

namespace model
{

class TextureLoader
{
public:
	static ur::TexturePtr
        LoadFromFile(const ur::Device& dev, const char* filepath, int mipmap_levels = 0);
	static ur::TexturePtr
        LoadFromMemory(const ur::Device& dev, const unsigned char* pixels, int width, int height, int channels);

}; // TextureLoader

}