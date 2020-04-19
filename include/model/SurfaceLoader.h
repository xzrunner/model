#pragma once

#include "model/Model.h"

#include <SM_Cube.h>

#include <string>
#include <memory>

namespace ur2 { class Device; }

namespace model
{

class SurfaceLoader
{
public:
	static bool Load(const ur2::Device& dev, Model& model, const std::string& filepath);

    static std::unique_ptr<Model::Mesh>
        CreateMesh(const ur2::Device& dev, const std::string& name, sm::cube& aabb);

}; // SurfaceLoader

}