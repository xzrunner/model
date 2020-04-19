#include "model/TextureLoader.h"

#include <guard/check.h>
#include <gimg_import.h>
#include <gimg_typedef.h>
#include <unirender2/Device.h>
#include <unirender2/Texture.h>
#include <unirender2/TextureFormat.h>
#include <unirender2/TextureDescription.h>

#include <boost/filesystem.hpp>

namespace model
{

ur2::TexturePtr
TextureLoader::LoadFromFile(const ur2::Device& dev, const char* filepath, int mipmap_levels)
{
	if (!boost::filesystem::is_regular_file(filepath)) {
		return false;
	}

	int w, h, fmt;
	uint8_t* pixels = gimg_import(filepath, &w, &h, &fmt);
	if (!pixels) {
		return false;
	}

	//if (tf == GPF_RGBA8 && gum::Config::Instance()->GetPreMulAlpha()) {
	//	gimg_pre_mul_alpha(pixels, w, h);
	//}

    ur2::TextureFormat tf;
	switch (fmt)
	{
	case GPF_ALPHA: case GPF_LUMINANCE: case GPF_LUMINANCE_ALPHA:
		tf = ur2::TextureFormat::A8;
		break;
    case GPF_RED:
        tf = ur2::TextureFormat::RED;
        break;
	case GPF_RGB:
		tf = ur2::TextureFormat::RGB;
		break;
	case GPF_RGBA8:
		tf = ur2::TextureFormat::RGBA8;
		break;
	case GPF_BGRA_EXT:
		tf = ur2::TextureFormat::BGRA_EXT;
		break;
	case GPF_BGR_EXT:
		tf = ur2::TextureFormat::BGR_EXT;
		break;
    case GPF_RGBA16F:
        tf = ur2::TextureFormat::RGBA16F;
        break;
    case GPF_RGB16F:
        tf = ur2::TextureFormat::RGB16F;
        break;
    case GPF_RGB32F:
        tf = ur2::TextureFormat::RGB32F;
        break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		tf = ur2::TextureFormat::COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		tf = ur2::TextureFormat::COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		tf = ur2::TextureFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		GD_REPORT_ASSERT("unknown type.");
	}

    return dev.CreateTexture({ tf });
}

ur2::TexturePtr
TextureLoader::LoadFromMemory(const ur2::Device& dev, const unsigned char* pixels, int width, int height, int channels)
{
	ur2::TextureFormat tf;
	switch (channels)
	{
	case 4:
        tf = ur2::TextureFormat::RGBA8;
		break;
	case 3:
        tf = ur2::TextureFormat::RGB;
		break;
	case 1:
        tf = ur2::TextureFormat::RED;
		break;
	default:
		assert(0);
	}
    return dev.CreateTexture({ tf });
}

}