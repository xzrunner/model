#pragma once

#include "model/Model.h"
#include "model/BrushModel.h"

#include <SM_Vector.h>
#include <polymesh3/typedef.h>

#include <memory>
#include <vector>

namespace model
{

class BrushBuilder
{
public:
    // polygon -> brush
    static std::unique_ptr<BrushModel>
        BrushFromPolygon(const std::vector<sm::vec3>& polygon);

    // brush -> polymesh
    static std::unique_ptr<Model>
        PolymeshFromBrush(const std::vector<std::shared_ptr<pm3::Brush>>& brushes);
    static std::unique_ptr<Model>
        PolymeshFromBrush(const model::BrushModel& brush);

    // polygon -> polymesh
    static std::unique_ptr<Model>
        PolymeshFromPolygon(const std::vector<sm::vec3>& polygon);

    static void UpdateVBO(Model& model, const BrushModel::BrushSingle& brush);

    // todo
public:
    struct Vertex
    {
        sm::vec3 pos;
        sm::vec3 normal;
        sm::vec2 texcoord;
    };

    static void CreateMeshRenderBuf(model::Model::Mesh& mesh,
        const std::vector<Vertex>& vertices);
    static void CreateBorderMeshRenderBuf(model::Model::Mesh& mesh,
        const std::vector<Vertex>& vertices, const std::vector<unsigned short>& indices);

    static Vertex CreateVertex(const pm3::BrushFacePtr& face, const sm::vec3& pos, int tex_w, int tex_h, sm::cube& aabb);
    static Vertex CreateVertex(const sm::vec3& pos, const sm::vec3& normal, sm::cube& aabb);

    static void FlushVertices(std::unique_ptr<model::Model::Mesh>& mesh,
        std::unique_ptr<model::Model::Mesh>& border_mesh,
        std::vector<Vertex>& vertices, std::vector<Vertex>& border_vertices,
        std::vector<unsigned short>& border_indices, model::Model& dst);

    static const float VERTEX_SCALE;

}; // BrushBuilder

}