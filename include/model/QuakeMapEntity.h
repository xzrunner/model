#ifndef NO_QUAKE

#pragma once

#include "model/BrushModel.h"

namespace quake { struct MapEntity; }

namespace model
{

class QuakeMapEntity : public ModelExtend
{
public:
	virtual ModelExtendType Type() const override { return EXT_QUAKE_MAP; }

    virtual std::unique_ptr<ModelExtend> Clone() const override;

	void SetMapEntity(const std::shared_ptr<quake::MapEntity>& entity) { m_map_entity = entity; }
	const std::shared_ptr<quake::MapEntity>& GetMapEntity() const { return m_map_entity; }

	void SetBrushDescs(const std::vector<BrushModel::BrushDesc>& brush_descs) {
		m_brush_descs = brush_descs;
	}
	auto& GetAllBrushDescs() const { return m_brush_descs; }

private:
    std::shared_ptr<quake::MapEntity> m_map_entity = nullptr;

	// update vbo from editor
	std::vector<BrushModel::BrushDesc> m_brush_descs;

}; // QuakeMapEntity

}

#endif // NO_QUAKE