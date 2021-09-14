#include "model/SurfaceLoader.h"
#include "model/SurfaceFactory.h"
#include "model/Surface.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/gltf/Model.h"

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

void SurfaceLoader::BuildPolymesh(const ur::Device& dev, const Surface& src, gltf::Model& dst)
{
    const int vertex_type = VERTEX_FLAG_NORMALS | VERTEX_FLAG_TEXCOORDS0;

    auto va = dev.CreateVertexArray();

    std::vector<unsigned short> indices;
    src.GenerateTriangleIndices(indices);
    auto ibuf_sz = sizeof(unsigned short) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->SetCount(indices.size());
    ibuf->Reserve(ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
    va->SetIndexBuffer(ibuf);

    std::vector<float> vertices;
    src.GenerateVertices(vertex_type, vertices);
    auto vbuf_sz = sizeof(float) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertices.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs(3);
    // pos
    vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(0, ur::ComponentDataType::Float, 3, 0, 32);
    // normal
    vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(1, ur::ComponentDataType::Float, 3, 12, 32);
    // texcoord
    vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(2, ur::ComponentDataType::Float, 2, 24, 32);
    va->SetVertexBufferAttrs(vbuf_attrs);

    auto d_prim = std::make_shared<gltf::Primitive>();
    d_prim->va = va;
    //d_prim->size = aabb.Size();
    d_prim->material = std::make_shared<gltf::Material>();
    auto d_mesh = std::make_shared<gltf::Mesh>();
    d_mesh->primitives.push_back(d_prim);
    auto d_node = std::make_shared<gltf::Node>();
    d_node->mesh = d_mesh;
    auto d_scene = std::make_shared<gltf::Scene>();
    d_scene->nodes.push_back(d_node);
    dst.scenes.push_back(d_scene);

    dst.scene = d_scene;
}

}