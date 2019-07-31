#include "model/BrushModel.h"

#include <polymesh3/Brush.h>

namespace model
{

std::unique_ptr<ModelExtend> BrushModel::Clone() const
{
    auto ret = std::make_unique<BrushModel>();

    ret->m_brushes.reserve(m_brushes.size());
    for (auto& s : m_brushes)
    {
        ret->m_brushes.push_back({
            s.desc, std::make_shared<pm3::Brush>(*s.impl)
        });
    }

    // todo
    //ret->m_brush_groups.reserve(m_brush_groups.size());
    //for (auto& s : m_brush_groups)
    //{
    //    ret->m_brush_groups.push_back({
    //        std::make_shared
    //    });
    //}

    return ret;
}

}