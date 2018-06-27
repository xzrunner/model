#include "model/Callback.h"

namespace model
{

static Callback::Funs FUNS;

void Callback::RegisterCallback(const Callback::Funs& funs)
{
	FUNS = funs;
}

void* Callback::CreateImg(const std::string& filepath)
{
	return FUNS.create_img_from_file(filepath);
}

void* Callback::CreateImg(const unsigned char* pixels, int width, int height, int channels)
{
	return FUNS.create_img_from_memory(pixels, width, height, channels);
}

void Callback::ReleaseImg(void* img)
{
	FUNS.release_img(img);
}

unsigned int Callback::GetTexID(const void* img)
{
	return FUNS.get_tex_id(img);
}

}