#pragma once

#include <string>

namespace model
{

struct Scene;

class SurfaceLoader
{
public:
	static bool Load(Scene& scene, const std::string& filepath);

}; // SurfaceLoader

}