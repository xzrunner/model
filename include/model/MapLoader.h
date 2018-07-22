#pragma once

#include <string>
#include <vector>

namespace quake { struct MapEntity; }

namespace model
{

struct Model;

class MapLoader
{
public:
	static void Load(std::vector<std::shared_ptr<Model>>& models, const std::string& filepath);

	static bool Load(Model& model, const std::string& filepath);

private:
	static void LoadTextures(const quake::MapEntity& world,
		const std::string& dir);

	static bool LoadEntity(Model& dst, const quake::MapEntity& src);

}; // MapLoader

}