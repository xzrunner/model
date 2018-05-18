#pragma once

#include <string>

namespace model
{

struct Model;

class SurfaceLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

}; // SurfaceLoader

}