#include "model/SurfaceLoader.h"
#include "model/SurfaceFactory.h"
#include "model/Surface.h"
#include "model/typedef.h"
#include "model/Model.h"

#include <unirender/Device.h>
#include <unirender/VertexArray.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexInputAttribute.h>

#include <filesystem>

namespace model
{

bool SurfaceLoader::Load(const ur::Device& dev, Model& model, const std::string& filepath)
{
    auto name = std::filesystem::path(filepath).stem().string();
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
SurfaceLoader::CreateMesh(const ur::Device& dev, const std::string& name, sm::cube& aabb)
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
    auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    std::vector<float> vertices;
    surface->GenerateVertices(vertex_type, vertices);
    auto vbuf_sz = sizeof(float) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs(2);
    // pos
    vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, 0, 24);
    // normal
    vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
        1, ur::ComponentDataType::Float, 3, 12, 24);
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