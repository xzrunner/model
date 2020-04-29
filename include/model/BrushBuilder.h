#pragma once

#include "model/Model.h"
#include "model/BrushModel.h"

#include <SM_Vector.h>
#include <polymesh3/Polytope.h>

#include <memory>
#include <vector>

namespace ur { class Device; }

namespace model
{

class BrushBuilder
{
public:
    // polygon -> brush
    static std::unique_ptr<BrushModel>
        BrushFromPolygon(const std::vector<sm::vec3>& polygon);

    // brush -> polymesh
    enum class VertexType
    {
        PosNorm,
        PosNormTex,
        PosNormCol,
    };
    static std::unique_ptr<Model> PolymeshFromBrushPN(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& brushes);
    static std::unique_ptr<Model> PolymeshFromBrushPN(const ur::Device& dev, const model::BrushModel& brush_model);
    static std::unique_ptr<Model> PolymeshFromBrushPNT(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& brushes,
        const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords
    );
    static std::unique_ptr<Model> PolymeshFromBrushPNT(const ur::Device& dev, const model::BrushModel& brush_model,
        const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords
    );
    static std::unique_ptr<Model> PolymeshFromBrushPNC(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& brushes,
        const std::vector<std::vector<std::vector<sm::vec3>>>& colors
    );
    static std::unique_ptr<Model> PolymeshFromBrushPNC(const ur::Device& dev, const model::BrushModel& brush_model,
        const std::vector<std::vector<std::vector<sm::vec3>>>& colors
    );
private:
    static std::unique_ptr<Model> PolymeshFromBrush(
        const ur::Device& dev, VertexType type, const std::vector<std::shared_ptr<pm3::Polytope>>& brushes,
        const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
        const std::vector<std::vector<std::vector<sm::vec3>>>& colors
    );
    static std::unique_ptr<Model> PolymeshFromBrush(
        const ur::Device& dev, VertexType type, const model::BrushModel& brush_model,
        const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
        const std::vector<std::vector<std::vector<sm::vec3>>>& colors
    );

public:

    // polygon -> polymesh
    static std::unique_ptr<Model>
        PolymeshFromPolygon(const ur::Device& dev, const std::vector<sm::vec3>& polygon);

    static void UpdateVBO(Model& model, const BrushModel::Brush& brush);
    static void UpdateVBO(Model& model, const model::BrushModel& brush_model);

public:
    struct Vertex
    {
        sm::vec3 pos;
        sm::vec3 normal;
        sm::vec2 texcoord;
        sm::vec3 color;
    };

    static void CreateMeshRenderBuf(const ur::Device& dev, VertexType type, model::Model::Mesh& mesh, const std::vector<Vertex>& vertices);
    static void CreateBorderMeshRenderBuf(const ur::Device& dev, VertexType type, model::Model::Mesh& mesh, const std::vector<Vertex>& vertices, const std::vector<unsigned short>& indices);

    static Vertex CreateVertex(const pm3::Polytope::FacePtr& face, const sm::vec3& pos,
        int tex_w, int tex_h, const sm::vec3& color, sm::cube& aabb);

    static void FlushVertices(const ur::Device& dev, VertexType type, std::unique_ptr<model::Model::Mesh>& mesh,
        std::unique_ptr<model::Model::Mesh>& border_mesh,
        std::vector<Vertex>& vertices, std::vector<Vertex>& border_vertices,
        std::vector<unsigned short>& border_indices, model::Model& dst);

private:
    static std::vector<size_t> Triangulation(const std::vector<pm3::Polytope::PointPtr>& verts,
        const std::vector<size_t>& border, const std::vector<std::vector<size_t>>& holes);

}; // BrushBuilder

}