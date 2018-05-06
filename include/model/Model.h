#pragma once

#include "model/Mesh.h"

#include <painting3/AABB.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <vector>

namespace model
{

class Model : boost::noncopyable
{
public:
	auto& GetAllMeshes() const { return m_meshes; }

	void AddMesh(std::unique_ptr<Mesh>& mesh) { m_meshes.push_back(std::move(mesh)); }

	auto& GetAABB() const { return m_aabb; }
	void SetAABB(const pt3::AABB& aabb) { m_aabb = aabb; }

	// for ResPool
	bool LoadFromFile(const std::string& filepath);

private:
	std::vector<std::unique_ptr<Mesh>> m_meshes;

	pt3::AABB m_aabb;

}; // Model

}