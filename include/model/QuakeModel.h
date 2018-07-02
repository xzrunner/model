#pragma once

#include <unirender/Texture.h>

#include <string.h>

namespace model
{

#define	MIPLEVELS	4

struct QuakeTexture
{
	char  name[16];
//	int   offsets[MIPLEVELS];

	ur::TexturePtr tex = nullptr;

};

}