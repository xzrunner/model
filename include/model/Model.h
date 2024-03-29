#pragma once

#include "model/MeshGeometry.h"
#include "model/SkeletalAnim.h"

#include <SM_Matrix.h>
#include <SM_Cube.h>
#include <unirender/typedef.h>
#include <unirender/noncopyable.h>

#include <vector>
#include <memory>

namespace ur { class Device; }

namespace model
{

struct Model : ur::noncopyable
{
    Model(const ur::Device* dev)
        : dev(dev)
    {
    }

	// for ResPool
	bool LoadFromFile(const std::string& filepath);

	struct Material
	{
		sm::vec3 ambient = { 0.04f, 0.04f, 0.04f };
		sm::vec3 diffuse = { 1, 1, 1 };
		sm::vec3 specular = { 1, 1, 1 };
		float    shininess = 50;

		int diffuse_tex = -1;
		int metallic_roughness_tex = -1;
		int emissive_tex = -1;
		int occlusion_tex = -1;
		int normal_tex = -1;
	};

	struct Mesh
	{
        std::string name;

		MeshGeometry geometry;

		int material = -1;

	}; // Mesh

	struct Node
	{
		sm::mat4 mat;

		int mesh = -1;
	};

	std::vector<std::pair<std::string, ur::TexturePtr>> textures;

	std::vector<std::unique_ptr<Material>> materials;

	std::vector<std::unique_ptr<Mesh>> meshes;
	std::vector<std::unique_ptr<Mesh>> border_meshes;

	std::unique_ptr<ModelExtend> ext = nullptr;

	std::vector<std::unique_ptr<Node>> nodes;

	sm::cube aabb;

	float anim_speed = 1;

	float scale = 1.0f;

    const ur::Device* dev = nullptr;

}; // Model

}