#pragma once

#include <SM_Vector.h>

#include <memory>
#include <vector>
#include <string>

namespace ur { class Texture; class TextureSampler; }

namespace model
{
namespace gltf
{

struct Texture
{
	std::shared_ptr<ur::Texture> image = nullptr;
	std::shared_ptr<ur::TextureSampler> sampler = nullptr;
};

struct Material
{
	struct Emissive
	{
		sm::vec3 factor = sm::vec3(1, 1, 1);

		std::shared_ptr<Texture> texture = nullptr;
		int tex_coord = 0;
	};

	struct Normal
	{
		std::shared_ptr<Texture> texture = nullptr;
		int tex_coord = 0;
	};

	struct Occlusion
	{
		std::shared_ptr<Texture> texture = nullptr;
		int tex_coord = 0;
	};

	struct MetallicRoughness
	{
		float metallic_factor = 1.0;
		float roughness_factor = 1.0;

		std::shared_ptr<Texture> texture = nullptr;
		int tex_coord = 0;
	};

	struct BaseColor
	{
		sm::vec4 factor = sm::vec4(1, 1, 1, 1);

		std::shared_ptr<Texture> texture = nullptr;
		int tex_coord = 0;
	};

	std::string name;

	std::shared_ptr<Emissive> emissive = nullptr;
	std::shared_ptr<Normal> normal = nullptr;
	std::shared_ptr<Occlusion> occlusion = nullptr;
	std::shared_ptr<MetallicRoughness> metallic_roughness = nullptr;
	std::shared_ptr<BaseColor> base_color = nullptr;
};

struct Primitive
{
	std::shared_ptr<Material> material = nullptr;
	std::shared_ptr<ur::VertexArray> va = nullptr;
};

struct Mesh
{
	std::string name;
	std::vector<std::shared_ptr<Primitive>> primitives;
};

struct Node
{
	std::string name;
	std::shared_ptr<Mesh> mesh = nullptr;
	sm::vec3 translation = sm::vec3(0, 0, 0);
	sm::vec4 rotation    = sm::vec4(0, 0, 0, 1);
	sm::vec3 scale       = sm::vec3(1, 1, 1);
};

struct Scene
{
	std::string name;
	std::vector<std::shared_ptr<Node>> nodes;
};

struct Model
{
	std::vector<std::shared_ptr<Scene>> scenes;
	std::shared_ptr<Scene> scene = nullptr;
};

}
}