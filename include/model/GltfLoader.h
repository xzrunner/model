#pragma once

#include <SM_Cube.h>

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace ur { class Device; class TextureSampler; class Texture; class VertexArray; }
namespace tinygltf { class Model; struct Image; struct Primitive; class Value; }

namespace model
{

struct Model;
namespace gltf { struct Model; struct Texture; struct Material; struct Mesh; struct Node; struct Scene; }

class GltfLoader
{
public:
	static bool Load(const ur::Device& dev, Model& model, const std::string& filepath);
	static bool Load(const ur::Device& dev, gltf::Model& model, const std::string& filepath);

private:
	static std::shared_ptr<ur::Texture> LoadTexture(const ur::Device& dev, const tinygltf::Image& img);
	static std::shared_ptr<ur::VertexArray> LoadVertexArray(const ur::Device& dev, 
		const tinygltf::Model& model, const tinygltf::Primitive& prim, unsigned int& vertex_type);

	static void LoadTextures(const ur::Device& dev, Model& dst, const tinygltf::Model& src);
	static void LoadMaterials(const ur::Device& dev, Model& dst, const tinygltf::Model& src);
	static void LoadMeshes(const ur::Device& dev, Model& dst, const tinygltf::Model& src, sm::cube& aabb);
	static void LoadNodes(const ur::Device& dev, Model& dst, const tinygltf::Model& src);

	static std::vector<std::shared_ptr<ur::TextureSampler>> LoadSamplers(
		const ur::Device& dev, const tinygltf::Model& model
	);
	static std::vector<std::shared_ptr<gltf::Texture>> LoadTextures(
		const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<ur::TextureSampler>>& samplers
	);
	static std::vector<std::shared_ptr<gltf::Material>> LoadMaterials(
		const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Texture>>& textures
	);
	static std::vector<std::shared_ptr<gltf::Mesh>> LoadMeshes(
		const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Material>>& materials
	);
	static std::vector<std::shared_ptr<gltf::Node>> LoadNodes(
		const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Mesh>>& meshes
	);
	static std::vector<std::shared_ptr<gltf::Scene>> LoadScenes(
		const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Node>>& nodes
	);
	static void LoadTextureTransform(gltf::Texture& dst, const tinygltf::Value& src);
	static void LoadTextureTransform(gltf::Texture& dst, const std::map<std::string, tinygltf::Value>& src);

}; // GltfLoader

}