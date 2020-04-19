#pragma once

#include "model/Model.h"

#include <SM_Vector.h>

#include <memory>

namespace ur2 { class Device; }

namespace model
{

class MeshBuider
{
public:
    static std::unique_ptr<Model::Mesh>
        CreateCube(const ur2::Device& dev, const sm::vec3& half_extents);

}; // MeshBuider

}