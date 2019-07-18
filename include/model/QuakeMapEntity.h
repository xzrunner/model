#pragma once

#include "model/ModelExtend.h"

#include <memory>

namespace quake { struct MapEntity; }

namespace model
{

class QuakeMapEntity : public ModelExtend
{
public:
	virtual ModelExtendType Type() const override { return EXT_QUAKE_MAP; }

	void SetMapEntity(const std::shared_ptr<quake::MapEntity>& entity) { m_map_entity = entity; }
	const std::shared_ptr<quake::MapEntity>& GetMapEntity() const { return m_map_entity; }

public:
	struct MeshDesc
	{
		int tex_width;
		int tex_height;

		int face_begin;
		int face_end;
	};

	struct BrushDesc
	{
		int mesh_begin;
		int mesh_end;

		std::vector<MeshDesc> meshes;
	};

	void SetBrushDescs(const std::vector<BrushDesc>& brush_descs) {
		m_brush_descs = brush_descs;
	}
	auto& GetAllBrushDescs() const { return m_brush_descs; }

private:
    std::shared_ptr<quake::MapEntity> m_map_entity = nullptr;

	// update vbo from editor
	std::vector<BrushDesc> m_brush_descs;

}; // QuakeMapEntity

}