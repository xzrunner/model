#pragma once

#include "model/Model.h"
#include "model/Mesh.h"

namespace pt3 { class AABB; }

namespace model
{

class Surface;

class ModelParametric : public Model
{
public:
	ModelParametric();
	ModelParametric(const Surface* surface);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual bool StoreToJson(rapidjson::Value& val,
		rapidjson::MemoryPoolAllocator<>& alloc) const override;
	virtual void LoadFromJson(const rapidjson::Value& val) override;

	// for ResPool
	bool LoadFromFile(const std::string& filepath);

	static const char* const TYPE_NAME;

private:
	 MeshPtr CreateMeshFromSurface(const Surface* surface);

}; // ModelParametric

}
