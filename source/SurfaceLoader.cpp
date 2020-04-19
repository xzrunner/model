#include "model/SurfaceLoader.h"
#include "model/SurfaceFactory.h"
#include "model/Surface.h"
#include "model/typedef.h"
#include "model/Model.h"

#include <unirender2/Device.h>
#include <unirender2/VertexArray.h>
#include <unirender2/IndexBuffer.h>
#include <unirender2/VertexBuffer.h>
#include <unirender2/VertexBufferAttribute.h>

#include <boost/filesystem.hpp>

namespace model
{

bool SurfaceLoader::Load(const ur2::Device& dev, Model& model, const std::string& filepath)
{
    auto name = boost::filesystem::path(filepath).stem().string();
    sm::cube aabb;
    auto mesh = CreateMesh(dev, name, aabb);
    if (!mesh) {
        return false;
    }

	model.materials.emplace_back(std::make_unique<Model::Material>());
	model.meshes.push_back(std::move(mesh));
	model.aabb = aabb;

	return true;
}

std::unique_ptr<Model::Mesh>
SurfaceLoader::CreateMesh(const ur2::Device& dev, const std::string& name, sm::cube& aabb)
{
	auto surface = SurfaceFactory::Create(name);
	if (!surface) {
		return nullptr;
	}

	const int vertex_type = VERTEX_FLAG_NORMALS;
	const int stride = 6;

    auto va = dev.CreateVertexArray();

    std::vector<unsigned short> indices;
    surface->GenerateTriangleIndices(indices);
    auto ibuf_sz = sizeof(unsigned short) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(ur2::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    std::vector<float> vertices;
    surface->GenerateVertices(vertex_type, vertices);
    auto vbuf_sz = sizeof(float) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur2::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur2::VertexBufferAttribute>> vbuf_attrs(2);
    // pos
    vbuf_attrs[0] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 0, 24);
    // normal
    vbuf_attrs[1] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 12, 24);
    va->SetVertexBufferAttrs(vbuf_attrs);

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
    mesh->geometry.vertex_array = va;
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, indices.size(), 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->material = 0;

	// aabb
	for (int i = 0, n = vertices.size(); i < n; )
	{
		sm::vec3 pos;
		pos.x = vertices[i];
		pos.y = vertices[i + 1];
		pos.z = vertices[i + 2];
		aabb.Combine(pos);
		i += stride;
	}

    return std::move(mesh);
}

}