#pragma once

#include "model/Model.h"

#include <string>

namespace ur { class Device; }
namespace tinygltf { class Model; }

namespace model
{

class GltfLoader
{
public:
	static bool Load(const ur::Device& dev, Model& model, const std::string& filepath);

private:
	static void LoadTextures(const ur::Device& dev, Model& dst, const tinygltf::Model& src);
	static void LoadMaterials(const ur::Device& dev, Model& dst, const tinygltf::Model& src);
	static void LoadMeshes(const ur::Device& dev, Model& dst, const tinygltf::Model& src, sm::cube& aabb);
	static void LoadNodes(const ur::Device& dev, Model& dst, const tinygltf::Model& src);

}; // GltfLoader

}