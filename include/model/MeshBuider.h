#pragma once

#include "model/Model.h"

#include <SM_Vector.h>

#include <memory>

namespace model
{

class MeshBuider
{
public:
    static std::unique_ptr<Model::Mesh> CreateCube(const sm::vec3& half_extents);

}; // MeshBuider

}