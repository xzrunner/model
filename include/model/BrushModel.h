#pragma once

#include "model/ModelExtend.h"

#include <polymesh3/Brush.h>

#include <vector>
#include <memory>

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

    struct BrushData
    {
        BrushDesc  desc;
        pm3::Brush impl;
    };

public:
    BrushModel() {}

    virtual ModelExtendType Type() const override { return EXT_BRUSH; }

    void  SetBrushes(const std::vector<BrushData>& brushes) { m_brushes = brushes; }
    auto& GetBrushes() const { return m_brushes; }

private:
    std::vector<BrushData> m_brushes;

}; // BrushModel

}