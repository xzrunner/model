#pragma once

#include <SM_Vector.h>

#include <string>
#include <vector>

namespace quake { struct MapEntity; }

namespace model
{

struct Model;

class MapBuilder
{
public:
	static std::shared_ptr<Model> Create(const std::vector<sm::vec3>& polygon);

	static void Load(std::vector<std::shared_ptr<Model>>& models, const std::string& filepath);

	static bool Load(Model& model, const std::string& filepath);

	static void UpdateVBO(Model& model, int brush_idx);

	static const float VERTEX_SCALE;

private:
	static void LoadTextures(const quake::MapEntity& world,
		const std::string& dir);

	static bool LoadEntity(Model& dst, const std::shared_ptr<quake::MapEntity>& src);

}; // MapBuilder

}