#pragma once

#include <SM_Vector.h>

#include <string>
#include <vector>

namespace model
{

struct BlendShapeData;
struct Model;

class BlendShapeLoader
{
public:
    struct MeshData
    {
        std::string name;
        std::vector<sm::vec3> vertices;
        std::vector<std::unique_ptr<BlendShapeData>> blendshapes;
    };

    static void Load(Model& model, std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes);

}; // BlendShapeLoader

}
