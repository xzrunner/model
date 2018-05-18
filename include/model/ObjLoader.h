// This code from d3d12book https://github.com/d3dcoder/d3d12book

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
