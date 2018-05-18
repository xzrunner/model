#include "model/SurfaceLoader.h"
#include "model/SurfaceFactory.h"
#include "model/Surface.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/EffectType.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

#include <boost/filesystem.hpp>

namespace model
{

bool SurfaceLoader::Load(Model& model, const std::string& filepath)
{
	auto name = boost::filesystem::path(filepath).stem().string();
	auto surface = SurfaceFactory::Create(name);
	if (!surface) {
		return false;
	}

	const int vertex_type = VERTEX_FLAG_NORMALS;
	const int stride = 6;

	ur::RenderContext::VertexInfo vi;

	std::vector<float> vertices;
	surface->GenerateVertices(vertex_type, vertices);
	vi.vn = vertices.size();
	vi.vertices = &vertices[0];

	std::vector<unsigned short> indices;
	surface->GenerateTriangleIndices(indices);
	vi.in = indices.size();
	vi.indices = &indices[0];

	vi.va_list.push_back(ur::RenderContext::VertexAttribute(0, 3, stride, 0));
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(1, 3, stride, 3));

	// material
	model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(vi.in, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->material = 0;
	mesh->effect = EFFECT_DEFAULT_NO_TEX;
	model.meshes.push_back(std::move(mesh));

	// node
	auto node = std::make_unique<Model::Node>();
	node->meshes.emplace_back(0);
	model.nodes.push_back(std::move(node));

	// aabb
	pt3::AABB aabb;
	for (int i = 0, n = vertices.size(); i < n; )
	{
		sm::vec3 pos;
		pos.x = vertices[i];
		pos.y = vertices[i + 1];
		pos.z = vertices[i + 2];
		aabb.Combine(pos);
		i += stride;
	}
	model.aabb = aabb;

	return true;
}

}