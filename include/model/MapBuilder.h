#pragma once

#include <SM_Vector.h>

#include <string>
#include <vector>

namespace quake { struct MapEntity; }
namespace ur2 { class Device; }

namespace model
{

struct Model;

class MapBuilder
{
public:
	static std::shared_ptr<Model>
        Create(const ur2::Device& dev, const std::vector<sm::vec3>& polygon);

	static void Load(const ur2::Device& dev, std::vector<std::shared_ptr<Model>>& models, const std::string& filepath);

	static bool Load(const ur2::Device& dev, Model& model, const std::string& filepath);

private:
	static void LoadTextures(const ur2::Device& dev,
        const quake::MapEntity& world, const std::string& dir);

	static bool LoadEntity(const ur2::Device& dev, Model& dst,
        const std::shared_ptr<quake::MapEntity>& src);

}; // MapBuilder

}