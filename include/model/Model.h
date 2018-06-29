#pragma once

#include "model/MeshGeometry.h"
#include "model/SkeletalAnim.h"

#include <painting3/AABB.h>
#include <SM_Matrix.h>

#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>

namespace model
{

struct Mesh;
struct Node;

struct Model : boost::noncopyable
{
	~Model();

	// for ResPool
	bool LoadFromFile(const std::string& filepath);

	struct Material
	{
		sm::vec3 ambient = { 0.04f, 0.04f, 0.04f };
		sm::vec3 diffuse = { 1, 1, 1 };
		sm::vec3 specular = { 1, 1, 1 };
		float    shininess = 50;

		int diffuse_tex = -1;
	};

	struct Mesh
	{
		MeshGeometry geometry;

		int material = -1;

		int effect = -1;

	}; // Mesh

	std::vector<std::pair<std::string, void*>> textures;

	std::vector<std::unique_ptr<Material>> materials;

	std::vector<std::unique_ptr<Mesh>> meshes;

	std::unique_ptr<Animation> anim = nullptr;

	pt3::AABB aabb;

	float anim_speed = 1;

}; // Model

}