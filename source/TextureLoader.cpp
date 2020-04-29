#include "model/TextureLoader.h"

#include <guard/check.h>
#include <gimg_import.h>
#include <gimg_typedef.h>
#include <unirender/Device.h>
#include <unirender/Texture.h>
#include <unirender/TextureFormat.h>
#include <unirender/TextureDescription.h>

#include <boost/filesystem.hpp>

namespace model
{

ur::TexturePtr
TextureLoader::LoadFromFile(const ur::Device& dev, const char* filepath, int mipmap_levels)
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

    ur::TextureFormat tf;
	switch (fmt)
	{
	case GPF_ALPHA: case GPF_LUMINANCE: case GPF_LUMINANCE_ALPHA:
		tf = ur::TextureFormat::A8;
		break;
    case GPF_RED:
        tf = ur::TextureFormat::RED;
        break;
	case GPF_RGB:
		tf = ur::TextureFormat::RGB;
		break;
	case GPF_RGBA8:
		tf = ur::TextureFormat::RGBA8;
		break;
	case GPF_BGRA_EXT:
		tf = ur::TextureFormat::BGRA_EXT;
		break;
	case GPF_BGR_EXT:
		tf = ur::TextureFormat::BGR_EXT;
		break;
    case GPF_RGBA16F:
        tf = ur::TextureFormat::RGBA16F;
        break;
    case GPF_RGB16F:
        tf = ur::TextureFormat::RGB16F;
        break;
    case GPF_RGB32F:
        tf = ur::TextureFormat::RGB32F;
        break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case GPF_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		tf = ur::TextureFormat::COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		GD_REPORT_ASSERT("unknown type.");
	}

    ur::TextureDescription desc;
    desc.target = ur::TextureTarget::Texture2D;
    desc.width  = w;
    desc.height = h;
    desc.format = tf;
    return dev.CreateTexture(desc);
}

ur::TexturePtr
TextureLoader::LoadFromMemory(const ur::Device& dev, const unsigned char* pixels, int width, int height, int channels)
{
	ur::TextureFormat tf;
	switch (channels)
	{
	case 4:
        tf = ur::TextureFormat::RGBA8;
		break;
	case 3:
        tf = ur::TextureFormat::RGB;
		break;
	case 1:
        tf = ur::TextureFormat::RED;
		break;
	default:
		assert(0);
	}

    ur::TextureDescription desc;
    desc.target = ur::TextureTarget::Texture2D;
    desc.width  = width;
    desc.height = height;
    desc.format = tf;
    return dev.CreateTexture(desc);
}

}