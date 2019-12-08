#include "model/BrushModel.h"

#include <polymesh3/Polytope.h>

namespace model
{

std::unique_ptr<ModelExtend> BrushModel::Clone() const
{
    auto ret = std::make_unique<BrushModel>();

    ret->m_brushes.reserve(m_brushes.size());
    for (auto& s : m_brushes)
    {
        ret->m_brushes.push_back({
            s.desc, std::make_shared<pm3::Polytope>(*s.impl)
        });
    }

    return ret;
}

}