#include "model/MapLoader.h"
#include "model/EffectType.h"
#include "model/MeshGeometry.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/HalfEdgeMesh.h"

#include <unirender/Blackboard.h>
#include <unirender/RenderContext.h>
#include <quake/MapParser.h>
#include <quake/WadFileLoader.h>
#include <quake/Palette.h>
#include <quake/TextureManager.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/filesystem.hpp>

#include <fstream>

namespace
{

struct Vertex
{
	sm::vec3 pos;
	sm::vec2 texcoord;
};

void CreateMeshRenderBuf(model::Model::Mesh& mesh, const std::vector<Vertex>& vertices)
{
	const int stride = sizeof(Vertex) / sizeof(float);

	ur::RenderContext::VertexInfo vi;

	vi.vn = vertices.size();
	vi.vertices = &vertices[0].pos.x;
	vi.stride = sizeof(Vertex);

	vi.in = 0;
	vi.indices = nullptr;

	vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 20, 0));	// pos
	vi.va_list.push_back(ur::VertexAttrib("texcoord", 2, 4, 20, 12));	// texcoord

	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
	int idx = mesh.geometry.sub_geometries.size();
	mesh.geometry.sub_geometry_materials.push_back(idx);
	mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(false, vi.vn, 0));
	mesh.geometry.vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
}

void CreateBorderMeshRenderBuf(model::Model::Mesh& mesh, const std::vector<Vertex>& vertices,
	                           const std::vector<unsigned short>& indices)
{
	const int stride = sizeof(Vertex) / sizeof(float);

	ur::RenderContext::VertexInfo vi;

	vi.vn = vertices.size();
	vi.vertices = &vertices[0].pos.x;
	vi.stride = sizeof(Vertex);

	vi.in = indices.size();
	vi.indices = &indices[0];

	vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 20, 0));	// pos
	vi.va_list.push_back(ur::VertexAttrib("texcoord", 2, 4, 20, 12));	// texcoord

	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
	int idx = mesh.geometry.sub_geometries.size();
	mesh.geometry.sub_geometry_materials.push_back(idx);
	mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(false, vi.vn, 0));
	mesh.geometry.vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
}

Vertex CreateVertex(const quake::BrushFace& face, const sm::vec3& pos, const ur::TexturePtr& tex, sm::cube& aabb)
{
	Vertex v;

	v.pos = pos * model::MapLoader::VERTEX_SCALE;
	aabb.Combine(v.pos);

	if (tex) {
		v.texcoord = face.CalcTexCoords(
			pos, static_cast<float>(tex->Width()), static_cast<float>(tex->Height()));
	} else {
		v.texcoord.Set(0, 0);
	}

	return v;
}

}

namespace model
{

const float MapLoader::VERTEX_SCALE = 0.01f;

void MapLoader::Load(std::vector<std::shared_ptr<Model>>& models, const std::string& filepath)
{
	std::ifstream fin(filepath);
	std::string str((std::istreambuf_iterator<char>(fin)),
		std::istreambuf_iterator<char>());
	fin.close();

	quake::MapParser parser(str);
	parser.Parse();
	for (auto& e : parser.GetAllEntities()) {
		for (auto& b : e->brushes) {
			b.BuildVertices();
			b.BuildGeometry();
		}
	}

	auto dir = boost::filesystem::path(filepath).parent_path().string();

	// load textures
	auto world = parser.GetWorldEntity();
	assert(world);
	LoadTextures(*world, dir);

	// load models
	models.clear();
	models.reserve(parser.GetAllEntities().size());
	for (auto& entity : parser.GetAllEntities())
	{
		auto model = std::make_shared<Model>();
		if (LoadEntity(*model, *entity)) {
			models.push_back(model);
		}
	}
}

bool MapLoader::Load(Model& model, const std::string& filepath)
{
	std::ifstream fin(filepath);
	std::string str((std::istreambuf_iterator<char>(fin)),
		std::istreambuf_iterator<char>());
	fin.close();

	quake::MapParser parser(str);
	parser.Parse();

	auto& entities = parser.GetAllEntities();
	if (entities.empty()) {
		return false;
	}

	for (auto& e : entities) {
		for (auto& b : e->brushes) {
			b.BuildVertices();
			b.BuildGeometry();
		}
	}

	auto dir = boost::filesystem::path(filepath).parent_path().string();

	// load textures
	auto world = parser.GetWorldEntity();
	assert(world);
	LoadTextures(*world, dir);

	// load model
	LoadEntity(model, *entities[0]);

	return true;
}

void MapLoader::LoadTextures(const quake::MapEntity& world, const std::string& dir)
{
	std::string tex_path;
	for (auto& attr : world.attributes)
	{
		if (attr.name == "wad") {
			tex_path = attr.val;
			break;
		}
	}

	assert(!tex_path.empty());
	std::vector<std::string> paths;
	boost::split(paths, tex_path, boost::is_any_of(";"));

	quake::Palette palette;
	quake::WadFileLoader loader(palette);
	for (auto& path : paths)
	{
		if (path.empty()) {
			continue;
		}
		auto full_path = boost::filesystem::absolute(path, dir);
		loader.Load(full_path.string());
	}
}

bool MapLoader::LoadEntity(Model& dst, const quake::MapEntity& src)
{
	if (src.brushes.empty()) {
		return false;
	}

	auto tex_mgr = quake::TextureManager::Instance();
	sm::cube aabb;
	for (auto& b : src.brushes)
	{
		if (b.faces.empty()) {
			continue;
		}

		// sort by texture
		auto faces = b.faces;
		std::sort(faces.begin(), faces.end(), [](const quake::BrushFace& lhs, const quake::BrushFace& rhs) {
			return lhs.tex_name < rhs.tex_name;
		});

		// create meshes
		std::unique_ptr<Model::Mesh> mesh = nullptr;
		std::unique_ptr<Model::Mesh> border_mesh = nullptr;
		std::vector<Vertex> vertices;
		std::vector<Vertex> border_vertices;
		std::vector<unsigned short> border_indices;
		std::string curr_tex_name;
		ur::TexturePtr curr_tex = nullptr;
		for (auto& f : faces)
		{
			// new material
			if (f.tex_name != curr_tex_name)
			{
				if (!vertices.empty())
				{
					CreateMeshRenderBuf(*mesh, vertices);
					dst.meshes.push_back(std::move(mesh));
					vertices.clear();

					CreateBorderMeshRenderBuf(*border_mesh, border_vertices, border_indices);
					dst.border_meshes.push_back(std::move(border_mesh));
					border_vertices.clear();
				}

				mesh = std::make_unique<Model::Mesh>();
				mesh->effect = EFFECT_USER;

				border_mesh = std::make_unique<Model::Mesh>();
				border_mesh->effect = EFFECT_USER;

				auto mat = std::make_unique<Model::Material>();
				int mat_idx = dst.materials.size();
				mesh->material = mat_idx;
				border_mesh->material = mat_idx;
				auto tex = tex_mgr->Query(f.tex_name);
				mat->diffuse_tex = dst.textures.size();
				dst.textures.push_back({ f.tex_name, tex });
				dst.materials.push_back(std::move(mat));

				curr_tex_name = f.tex_name;
				curr_tex = tex;
			}

			assert(f.vertices.size() > 2);
			for (size_t i = 1; i < f.vertices.size() - 1; ++i)
			{
				vertices.push_back(CreateVertex(f, f.vertices[0], curr_tex, aabb));
				vertices.push_back(CreateVertex(f, f.vertices[i], curr_tex, aabb));
				vertices.push_back(CreateVertex(f, f.vertices[i + 1], curr_tex, aabb));
			}

			int start_idx = border_vertices.size();
			for (auto& v : f.vertices) {
				border_vertices.push_back(CreateVertex(f, v, curr_tex, aabb));
			}
			for (int i = 0, n = f.vertices.size(); i < n; ++i) {
				border_indices.push_back(i);
				border_indices.push_back(i + 1);
			}
			border_indices.push_back(static_cast<unsigned short>(f.vertices.size() - 1));
			border_indices.push_back(start_idx);
		}

		if (!vertices.empty())
		{
			CreateMeshRenderBuf(*mesh, vertices);
			dst.meshes.push_back(std::move(mesh));
			vertices.clear();

			CreateBorderMeshRenderBuf(*border_mesh, border_vertices, border_indices);
			dst.border_meshes.push_back(std::move(border_mesh));
			border_vertices.clear();
		}
	}

	dst.aabb = aabb;
	aabb.MakeEmpty();

	auto mesh = std::make_unique<HalfEdgeMesh>();
	mesh->meshes.reserve(src.brushes.size());
	for (auto& brush : src.brushes) {
		mesh->meshes.push_back(brush.geometry);
	}
	dst.ext = std::move(mesh);

	return true;
}

}