#include "model/SurfaceLoader.h"
#include "model/SurfaceFactory.h"
#include "model/Surface.h"
#include "model/typedef.h"
#include "model/Model.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>
#include <unirender/VertexAttrib.h>

#include <boost/filesystem.hpp>

namespace model
{

bool SurfaceLoader::Load(Model& model, const std::string& filepath)
{
    auto name = boost::filesystem::path(filepath).stem().string();
    sm::cube aabb;
    auto mesh = CreateMesh(name, aabb);
    if (!mesh) {
        return false;
    }

	model.materials.emplace_back(std::make_unique<Model::Material>());
	model.meshes.push_back(std::move(mesh));
	model.aabb = aabb;

	return true;
}

std::unique_ptr<Model::Mesh> SurfaceLoader::CreateMesh(const std::string& name, sm::cube& aabb)
{
	auto surface = SurfaceFactory::Create(name);
	if (!surface) {
		return nullptr;
	}

	const int vertex_type = VERTEX_FLAG_NORMALS;
	const int stride = 6;

	ur::RenderContext::VertexInfo vi;

	std::vector<float> vertices;
	surface->GenerateVertices(vertex_type, vertices);
	vi.vn = vertices.size() / stride;
	vi.vertices = &vertices[0];
	vi.stride = stride * sizeof(float);

	std::vector<unsigned short> indices;
	surface->GenerateTriangleIndices(indices);
	vi.in = indices.size();
	vi.indices = &indices[0];

	vi.va_list.push_back(ur::VertexAttrib("pos",    3, 4, 24, 0));
	vi.va_list.push_back(ur::VertexAttrib("normal", 3, 4, 24, 12));

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, vi.in, 0));
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