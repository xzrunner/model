#pragma once

#include "model/ModelExtend.h"

#include <polymesh3/typedef.h>

#include <vector>
#include <memory>

namespace pm3 { class Polytope; }

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

    struct Brush
    {
        BrushDesc        desc;
        pm3::PolytopePtr impl = nullptr;
    };

public:
    BrushModel() {}

    virtual ModelExtendType Type() const override { return EXT_BRUSH; }

    virtual std::unique_ptr<ModelExtend> Clone() const override;

    void  SetBrushes(const std::vector<Brush>& brushes) { m_brushes = brushes; }
    auto& GetBrushes() const { return m_brushes; }

private:
    std::vector<Brush> m_brushes;

}; // BrushModel

}