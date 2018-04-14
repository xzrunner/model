#pragma once

#include <string>

namespace model
{

class Surface;

class SurfaceFactory
{
public:
	static Surface* Create(const std::string& name);

}; // SurfaceFactory

}