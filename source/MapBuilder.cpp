#include "model/MapBuilder.h"
#include "model/MeshGeometry.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/QuakeMapEntity.h"

#include <unirender/Blackboard.h>
#include <unirender/RenderContext.h>
#include <polymesh3/Brush.h>
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

void CreateMeshRenderBuf(model::Model::Mesh& mesh,
	                     const std::vector<Vertex>& vertices)
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

void CreateBorderMeshRenderBuf(model::Model::Mesh& mesh,
	                           const std::vector<Vertex>& vertices,
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
	mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(true, vi.in, 0));
	mesh.geometry.vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
}

Vertex CreateVertex(const pm3::BrushFacePtr& face, const sm::vec3& pos,
	                int tex_w, int tex_h, sm::cube& aabb)
{
	Vertex v;

	v.pos = pos * model::MapBuilder::VERTEX_SCALE;
	aabb.Combine(v.pos);

	if (tex_w == 0 || tex_h == 0) {
		v.texcoord.Set(0, 0);
	} else {
		v.texcoord = face->CalcTexCoords(
			pos, static_cast<float>(tex_w), static_cast<float>(tex_h));
	}

	return v;
}

Vertex CreateVertex(const sm::vec3& pos, sm::cube& aabb)
{
	Vertex v;

	v.pos = pos * model::MapBuilder::VERTEX_SCALE;
	aabb.Combine(v.pos);

	v.texcoord.Set(0, 0);

	return v;
}

void FlushVertices(std::unique_ptr<model::Model::Mesh>& mesh,
	               std::unique_ptr<model::Model::Mesh>& border_mesh,
	               std::vector<Vertex>& vertices,
	               std::vector<Vertex>& border_vertices,
	               std::vector<unsigned short>& border_indices,
	               model::Model& dst)
{
	CreateMeshRenderBuf(*mesh, vertices);
	dst.meshes.push_back(std::move(mesh));
	vertices.clear();

	CreateBorderMeshRenderBuf(*border_mesh, border_vertices, border_indices);
	dst.border_meshes.push_back(std::move(border_mesh));
	border_vertices.clear();
}

void FlushBrushDesc(model::QuakeMapEntity::BrushDesc& brush_desc,
	                model::QuakeMapEntity::MeshDesc& mesh_desc,
	                int face_idx)
{
	mesh_desc.face_end = face_idx;
	brush_desc.meshes.push_back(mesh_desc);
	mesh_desc.face_begin = mesh_desc.face_end;
}

}

namespace model
{

const float MapBuilder::VERTEX_SCALE = 0.01f;

std::shared_ptr<Model> MapBuilder::Create(const std::vector<sm::vec3>& polygon)
{
	if (polygon.size() < 3) {
		return nullptr;
	}

	// scale
	auto scaled_poly = polygon;
	const float scale = 1.0f / MapBuilder::VERTEX_SCALE;
	for (auto& v : scaled_poly) {
		v *= scale;
	}

	// fix dir
	// should be clockwise, as left-hand system, top face's normal direction is y
	auto v0 = polygon[1] - polygon[0];
	auto v1 = polygon[2] - polygon[1];
	if (v0.Cross(v1).Dot({ 0, 1, 0 }) < 0) {
		std::reverse(scaled_poly.begin(), scaled_poly.end());
	}

	const float dy = 0.1f;
//	const float dy = 30;

	auto model = std::make_shared<Model>();

	// quake map brush data

	auto map_entity = std::make_unique<QuakeMapEntity>();
	QuakeMapEntity::BrushDesc brush_desc;
	brush_desc.mesh_begin = 0;
	brush_desc.mesh_end   = 1;
	int face_num = scaled_poly.size() + 2;
	brush_desc.meshes.push_back({ 0, 0, 0, face_num });
	map_entity->SetBrushDescs({ brush_desc });

	std::vector<std::shared_ptr<pm3::BrushFace>> faces;
	faces.reserve(face_num);
	// top
	{
		sm::vec3 tri[3];
		tri[0] = scaled_poly[0];
		tri[1] = scaled_poly[1];
		tri[2] = scaled_poly[2];
		for (int i = 0; i < 3; ++i) {
			tri[i].y += dy;
		}
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(tri[0], tri[1], tri[2]);
		faces.push_back(face);
	}
	// bottom
	{
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(scaled_poly[2], scaled_poly[1], scaled_poly[0]);
		faces.push_back(face);
	}
	// edge faces
	for (size_t i = 0, n = scaled_poly.size(); i < n; ++i)
	{
		auto& v0 = scaled_poly[i];
		auto& v1 = scaled_poly[(i + 1) % scaled_poly.size()];
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(v0, v1, { v1.x, v1.y + dy, v1.z });
		faces.push_back(face);
	}

	pm3::Brush brush(faces);
	brush.BuildVertices();
	brush.BuildGeometry();

	auto entity = std::make_shared<quake::MapEntity>();
	entity->brushes.push_back(brush);
	map_entity->SetMapEntity(entity);

	model->ext = std::move(map_entity);

	// mesh

	std::unique_ptr<Model::Mesh> mesh = nullptr;
	std::unique_ptr<Model::Mesh> border_mesh = nullptr;

	mesh = std::make_unique<Model::Mesh>();

	border_mesh = std::make_unique<Model::Mesh>();

	auto mat = std::make_unique<Model::Material>();
	int mat_idx = model->materials.size();
	mesh->material = mat_idx;
	border_mesh->material = mat_idx;
	mat->diffuse_tex = -1;
	model->materials.push_back(std::move(mat));

	std::vector<Vertex> vertices;
	std::vector<Vertex> border_vertices;
	std::vector<unsigned short> border_indices;

	sm::cube aabb;
	int start_idx = 0;
	for (auto& f : faces)
	{
		for (size_t i = 1; i < f->vertices.size() - 1; ++i)
		{
			vertices.push_back(CreateVertex(f->vertices[0]->pos, aabb));
			vertices.push_back(CreateVertex(f->vertices[i]->pos, aabb));
			vertices.push_back(CreateVertex(f->vertices[i + 1]->pos, aabb));
		}
		for (auto& vert : f->vertices) {
			border_vertices.push_back(CreateVertex(vert->pos, aabb));
		}
		for (int i = 0, n = f->vertices.size() - 1; i < n; ++i) {
			border_indices.push_back(start_idx + i);
			border_indices.push_back(start_idx + i + 1);
		}
		border_indices.push_back(static_cast<unsigned short>(start_idx + f->vertices.size() - 1));
		border_indices.push_back(start_idx);
		start_idx += f->vertices.size();
	}
	if (!vertices.empty()) {
		FlushVertices(mesh, border_mesh, vertices, border_vertices, border_indices, *model);
	}
	model->aabb = aabb;

	return model;
}

void MapBuilder::Load(std::vector<std::shared_ptr<Model>>& models, const std::string& filepath)
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
		if (LoadEntity(*model, entity)) {
			models.push_back(model);
		}
	}
}

bool MapBuilder::Load(Model& model, const std::string& filepath)
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
	LoadEntity(model, entities[0]);

	return true;
}

void MapBuilder::UpdateVBO(Model& model, int brush_idx)
{
	auto& rc = ur::Blackboard::Instance()->GetRenderContext();

	assert(model.ext && model.ext->Type() == EXT_QUAKE_MAP);
	auto map_entity = static_cast<QuakeMapEntity*>(model.ext.get());
	auto& brushes = map_entity->GetMapEntity()->brushes;
	assert(brush_idx >= 0 && brush_idx < static_cast<int>(brushes.size()));
	auto& descs = map_entity->GetAllBrushDescs();
	assert(descs.size() == brushes.size());

	auto& faces = brushes[brush_idx].faces;
	auto& desc = descs[brush_idx];
	assert(desc.mesh_end - desc.mesh_begin == desc.meshes.size());
	// traverse brush's meshes
	for (int i = desc.mesh_begin; i < desc.mesh_end; ++i)
	{
		// gen mesh's vertex buffer
		std::vector<Vertex> vertices, border_vertices;
		for (auto& src_mesh : desc.meshes)
		{
			int tex_w = src_mesh.tex_width;
			int tex_h = src_mesh.tex_height;
			sm::cube aabb; // todo
			for (int j = src_mesh.face_begin; j < src_mesh.face_end; ++j)
			{
				auto& f = faces[j];
				std::vector<sm::vec3> border;
				assert(f->vertices.size() > 2);
				for (size_t i = 1; i < f->vertices.size() - 1; ++i)
				{
					vertices.push_back(CreateVertex(f, f->vertices[0]->pos, tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, f->vertices[i]->pos, tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, f->vertices[i + 1]->pos, tex_w, tex_h, aabb));
				}
				for (auto& vert : f->vertices) {
					border_vertices.push_back(CreateVertex(f, vert->pos, tex_w, tex_h, aabb));
				}
			}
		}

		// upload buffer data
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.meshes[i]->geometry.vbo, vertices.data(),
			sizeof(Vertex) * vertices.size());
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.border_meshes[i]->geometry.vbo, border_vertices.data(),
			sizeof(Vertex) * border_vertices.size());

	}
}

void MapBuilder::LoadTextures(const quake::MapEntity& world, const std::string& dir)
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

bool MapBuilder::LoadEntity(Model& dst, const std::shared_ptr<quake::MapEntity>& src)
{
	if (src->brushes.empty()) {
		return false;
	}

	auto map_entity = std::make_unique<QuakeMapEntity>();

	std::vector<QuakeMapEntity::BrushDesc> brush_descs;
	brush_descs.reserve(src->brushes.size());

	auto tex_mgr = quake::TextureManager::Instance();
	sm::cube aabb;
	for (auto& b : src->brushes)
	{
		QuakeMapEntity::BrushDesc brush_desc;

		brush_desc.mesh_begin = dst.meshes.size();

		if (b.faces.empty()) {
			brush_desc.mesh_end = dst.meshes.size();
			brush_descs.push_back(brush_desc);
			continue;
		}

		QuakeMapEntity::MeshDesc mesh_desc;
		mesh_desc.face_begin = 0;

		// sort by texture
		auto faces = b.faces;
		std::sort(faces.begin(), faces.end(),
			[](const pm3::BrushFacePtr& lhs, const pm3::BrushFacePtr& rhs) {
			return lhs->tex_name < rhs->tex_name;
		});

		// create meshes
		std::unique_ptr<Model::Mesh> mesh = nullptr;
		std::unique_ptr<Model::Mesh> border_mesh = nullptr;
		std::vector<Vertex> vertices;
		std::vector<Vertex> border_vertices;
		std::vector<unsigned short> border_indices;
		std::string curr_tex_name;
		ur::TexturePtr curr_tex = nullptr;
		int face_idx = 0;
		for (auto& f : faces)
		{
			// new material
			if (f->tex_name != curr_tex_name)
			{
				if (!vertices.empty()) {
					FlushVertices(mesh, border_mesh, vertices, border_vertices, border_indices, dst);
					FlushBrushDesc(brush_desc, mesh_desc, face_idx);
				}

				mesh = std::make_unique<Model::Mesh>();

				border_mesh = std::make_unique<Model::Mesh>();

				auto mat = std::make_unique<Model::Material>();
				int mat_idx = dst.materials.size();
				mesh->material = mat_idx;
				border_mesh->material = mat_idx;
				auto tex = tex_mgr->Query(f->tex_name);
				mat->diffuse_tex = dst.textures.size();
				dst.textures.push_back({ f->tex_name, tex });
				dst.materials.push_back(std::move(mat));

				curr_tex_name = f->tex_name;
				curr_tex = tex;
			}

			int tex_w = 0, tex_h = 0;
			if (curr_tex) {
				tex_w = curr_tex->Width();
				tex_h = curr_tex->Height();
			}
			mesh_desc.tex_width  = tex_w;
			mesh_desc.tex_height = tex_h;

			assert(f->vertices.size() > 2);
			for (size_t i = 1; i < f->vertices.size() - 1; ++i)
			{
				vertices.push_back(CreateVertex(f, f->vertices[0]->pos, tex_w, tex_h, aabb));
				vertices.push_back(CreateVertex(f, f->vertices[i]->pos, tex_w, tex_h, aabb));
				vertices.push_back(CreateVertex(f, f->vertices[i + 1]->pos, tex_w, tex_h, aabb));
			}

			int start_idx = border_vertices.size();
			for (auto& v : f->vertices) {
				border_vertices.push_back(CreateVertex(f, v->pos, tex_w, tex_h, aabb));
			}
			for (int i = 0, n = f->vertices.size() - 1; i < n; ++i) {
				border_indices.push_back(start_idx + i);
				border_indices.push_back(start_idx + i + 1);
			}
			border_indices.push_back(start_idx + static_cast<unsigned short>(f->vertices.size() - 1));
			border_indices.push_back(start_idx);

			++face_idx;
		}

		if (!vertices.empty()) {
			FlushVertices(mesh, border_mesh, vertices, border_vertices, border_indices, dst);
			FlushBrushDesc(brush_desc, mesh_desc, face_idx);
		}

		brush_desc.mesh_end = dst.meshes.size();
		assert(brush_desc.mesh_end - brush_desc.mesh_begin == brush_desc.meshes.size());
		brush_descs.push_back(brush_desc);
	}

	dst.aabb = aabb;
	aabb.MakeEmpty();

	map_entity->SetMapEntity(src);
	map_entity->SetBrushDescs(brush_descs);

	dst.ext = std::move(map_entity);

	return true;
}

}