#pragma once

#include "model/Model.h"

#include <SM_Cube.h>

#include <string>
#include <memory>

namespace model
{

class SurfaceLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

    static std::unique_ptr<Model::Mesh> CreateMesh(const std::string& name, sm::cube& aabb);

}; // SurfaceLoader

}