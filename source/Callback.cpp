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
	return FUNS.create_img(filepath);
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