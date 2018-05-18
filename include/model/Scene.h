#pragma once

#include "MeshGeometry.h"

#include <painting3/AABB.h>
#include <SM_Matrix.h>

#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>

namespace model
{

struct Mesh;
struct Node;

struct Scene : boost::noncopyable
{
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

	struct Node : boost::noncopyable
	{
		std::string name;

		int parent = -1;
		std::vector<int> children;

		std::vector<int> meshes;

		sm::mat4 local_mat;

	}; // Node

	std::vector<std::pair<std::string, void*>> textures;

	std::vector<std::unique_ptr<Material>> materials;

	std::vector<std::unique_ptr<Mesh>> meshes;

	std::vector<std::unique_ptr<Node>> nodes;

	pt3::AABB aabb;

	// for ResPool
	bool LoadFromFile(const std::string& filepath);

}; // Scene

}