#pragma once

#include "model/Mesh.h"

#include <rapidjson/document.h>

#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>

namespace model
{

class Mesh;

class Model : boost::noncopyable
{
public:
	virtual const char* Type() const = 0;

	virtual bool StoreToJson(rapidjson::Value& val,
		rapidjson::MemoryPoolAllocator<>& alloc) const = 0;
	virtual void LoadFromJson(const rapidjson::Value& val) = 0;

	const std::vector<MeshPtr>& GetAllMeshes() const { return m_meshes; }

	void AddMesh(const MeshPtr& mesh) { m_meshes.push_back(mesh); }

protected:
	std::vector<MeshPtr> m_meshes;
	
}; // Model

}
