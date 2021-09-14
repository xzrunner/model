#pragma once

#include "model/Model.h"

#include <SM_Cube.h>

#include <string>
#include <memory>

namespace ur { class Device; }

namespace model
{

class Surface;

namespace gltf { struct Model; }

class SurfaceLoader
{
public:
	static bool Load(const ur::Device& dev, Model& model, const std::string& filepath);

    static std::unique_ptr<Model::Mesh>
        CreateMesh(const ur::Device& dev, const std::string& name, sm::cube& aabb);

    static void BuildPolymesh(const ur::Device& dev, const Surface& src, gltf::Model& dst);

}; // SurfaceLoader

}