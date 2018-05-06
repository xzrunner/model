#pragma once

#include <string>
#include <functional>

namespace model
{

class Callback
{
public:
	struct Funs
	{
		std::function<void*(const std::string&)> create_img;
		std::function<void (void*)> release_img;
		std::function<unsigned int(const void*)> get_tex_id;
	};

	static void RegisterCallback(const Funs& funs);

	static void* CreateImg(const std::string& filepath);
	static void ReleaseImg(void* img);
	static unsigned int GetTexID(const void* img);

}; // Callback

}