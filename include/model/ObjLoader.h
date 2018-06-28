#pragma once

#include <string>

namespace model
{

struct Model;

class ObjLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

}; // ObjLoader

}
