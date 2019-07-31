#include "model/QuakeMapEntity.h"

#include <polymesh3/Brush.h>
#include <quake/MapEntity.h>

namespace model
{

std::unique_ptr<ModelExtend> QuakeMapEntity::Clone() const
{
    auto ret = std::make_unique<QuakeMapEntity>();

    ret->m_map_entity = std::make_shared<quake::MapEntity>();

    ret->m_map_entity->attributes = m_map_entity->attributes;

    ret->m_map_entity->brushes.reserve(m_map_entity->brushes.size());
    for (auto& b : m_map_entity->brushes) {
        ret->m_map_entity->brushes.push_back(std::make_shared<pm3::Brush>(*b));
    }

    ret->m_brush_descs = m_brush_descs;

    return ret;
}

}