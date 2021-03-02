#pragma once

#include <SM_Vector.h>

#include <string>
#include <vector>
#include <memory>

namespace model
{

struct BlendShapeData;
struct Model;

class BlendShapeLoader
{
public:
    struct VertBuf
    {
        std::string name;
        std::vector<sm::vec3> verts;
    };

    struct MeshData
    {
        std::unique_ptr<VertBuf> mesh;
        std::vector<std::unique_ptr<VertBuf>> blendshapes;
    };

    static void Load(Model& model, std::vector<std::unique_ptr<MeshData>>& meshes);

}; // BlendShapeLoader

}
