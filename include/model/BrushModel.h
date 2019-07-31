#pragma once

#include "model/ModelExtend.h"

#include <polymesh3/typedef.h>

#include <vector>
#include <memory>

namespace pm3 { struct Brush; }

namespace model
{

class BrushModel : public ModelExtend
{
public:
    struct MeshDesc
    {
        int tex_width;
        int tex_height;

        int face_begin;
        int face_end;
    };

    // update vbo from editor
    struct BrushDesc
    {
        int mesh_begin;
        int mesh_end;

        std::vector<MeshDesc> meshes;
    };

    struct BrushSingle
    {
        BrushDesc                   desc;
        std::shared_ptr<pm3::Brush> impl = nullptr;
    };

    typedef size_t                    BrushVertex;
    typedef std::pair<size_t, size_t> BrushEdge;

    typedef std::shared_ptr<BrushVertex> BrushVertexPtr;
    typedef std::shared_ptr<BrushEdge>   BrushEdgePtr;

    struct BrushPart
    {
        std::shared_ptr<pm3::Brush> parent = nullptr;

        std::vector<BrushVertex>       vertices;
        std::vector<BrushEdge>         edges;
        std::vector<pm3::BrushFacePtr> faces;

    }; // BrushPart

    struct BrushGroup
    {
        std::string name;
        BrushPart   part;

    }; // BrushGroup

public:
    BrushModel() {}

    virtual ModelExtendType Type() const override { return EXT_BRUSH; }

    virtual std::unique_ptr<ModelExtend> Clone() const override;

    void  SetBrushes(const std::vector<BrushSingle>& brushes) { m_brushes = brushes; }
    auto& GetBrushes() const { return m_brushes; }

    void  SetBrushGroups(const std::vector<std::shared_ptr<BrushGroup>>& groups) { m_brush_groups = groups; }
    auto& GetBrushGroups() const { return m_brush_groups; }

private:
    std::vector<BrushSingle> m_brushes;

    std::vector<std::shared_ptr<BrushGroup>> m_brush_groups;

}; // BrushModel

}