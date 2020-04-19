#pragma once

#include <unirender2/typedef.h>

namespace ur2 { class Device; }

namespace model
{

class TextureLoader
{
public:
	static ur2::TexturePtr
        LoadFromFile(const ur2::Device& dev, const char* filepath, int mipmap_levels = 0);
	static ur2::TexturePtr
        LoadFromMemory(const ur2::Device& dev, const unsigned char* pixels, int width, int height, int channels);

}; // TextureLoader

}