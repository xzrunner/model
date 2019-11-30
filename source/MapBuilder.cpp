#include "model/MapBuilder.h"
#include "model/QuakeMapEntity.h"
#include "model/BrushBuilder.h"
#include "model/BrushModel.h"

#include <polymesh3/Geometry.h>
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

void FlushBrushDesc(model::BrushModel::BrushDesc& brush_desc,
                    model::BrushModel::MeshDesc& mesh_desc,
                    int face_idx)
{
	mesh_desc.face_end = face_idx;
	brush_desc.meshes.push_back(mesh_desc);
	mesh_desc.face_begin = mesh_desc.face_end;
}

}

namespace model
{

std::shared_ptr<Model> MapBuilder::Create(const std::vector<sm::vec3>& polygon)
{
    auto brush_model = BrushBuilder::BrushFromPolygon(polygon);
    if (!brush_model) {
        return nullptr;
    }

	auto map_entity = std::make_unique<QuakeMapEntity>();
    BrushModel::BrushDesc brush_desc;
	brush_desc.mesh_begin = 0;
	brush_desc.mesh_end   = 1;
	int face_num = polygon.size() + 2;
	brush_desc.meshes.push_back({ 0, 0, 0, face_num });
	map_entity->SetBrushDescs({ brush_desc });

	auto entity = std::make_shared<quake::MapEntity>();
    auto& brushes = brush_model->GetBrushes();
    assert(brushes.size() == 1);
    entity->brushes.push_back(brushes[0].impl);
	map_entity->SetMapEntity(entity);

    auto model = BrushBuilder::PolymeshFromBrushPN(*brush_model);
    assert(model);
	model->ext = std::move(map_entity);

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
			b->Build();
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
            b->Build();
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

	std::vector<BrushModel::BrushDesc> brush_descs;
	brush_descs.reserve(src->brushes.size());

	auto tex_mgr = quake::TextureManager::Instance();
	sm::cube aabb;
	for (auto& b : src->brushes)
	{
        BrushModel::BrushDesc brush_desc;

		brush_desc.mesh_begin = dst.meshes.size();

		if (b->Faces().empty()) {
			brush_desc.mesh_end = dst.meshes.size();
			brush_descs.push_back(brush_desc);
			continue;
		}

		BrushModel::MeshDesc mesh_desc;
		mesh_desc.face_begin = 0;

		// sort by texture
		auto faces = b->Faces();
		std::sort(faces.begin(), faces.end(),
			[](const pm3::FacePtr& lhs, const pm3::FacePtr& rhs) {
			return lhs->tex_map.tex_name < rhs->tex_map.tex_name;
		});

		// create meshes
		std::unique_ptr<Model::Mesh> mesh = nullptr;
		std::unique_ptr<Model::Mesh> border_mesh = nullptr;
		std::vector<BrushBuilder::Vertex> vertices;
		std::vector<BrushBuilder::Vertex> border_vertices;
		std::vector<unsigned short> border_indices;
		std::string curr_tex_name;
		ur::TexturePtr curr_tex = nullptr;
		int face_idx = 0;
		for (auto& f : faces)
		{
			// new material
			if (f->tex_map.tex_name != curr_tex_name)
			{
				if (!vertices.empty()) {
                    BrushBuilder::FlushVertices(BrushBuilder::VertexType::PosNormTex,
                        mesh, border_mesh, vertices, border_vertices, border_indices, dst);
					FlushBrushDesc(brush_desc, mesh_desc, face_idx);
				}

				mesh = std::make_unique<Model::Mesh>();

				border_mesh = std::make_unique<Model::Mesh>();

				auto mat = std::make_unique<Model::Material>();
				int mat_idx = dst.materials.size();
				mesh->material = mat_idx;
				border_mesh->material = mat_idx;
				auto tex = tex_mgr->Query(f->tex_map.tex_name);
				mat->diffuse_tex = dst.textures.size();
				dst.textures.push_back({ f->tex_map.tex_name, tex });
				dst.materials.push_back(std::move(mat));

				curr_tex_name = f->tex_map.tex_name;
				curr_tex = tex;
			}

			int tex_w = 0, tex_h = 0;
			if (curr_tex) {
				tex_w = curr_tex->Width();
				tex_h = curr_tex->Height();
			}
			mesh_desc.tex_width  = tex_w;
			mesh_desc.tex_height = tex_h;

            static const sm::vec3 WHITE(1.0f, 1.0f, 1.0f);

			assert(f->points.size() > 2);
			for (size_t i = 1; i < f->points.size() - 1; ++i)
			{
				vertices.push_back(BrushBuilder::CreateVertex(
                    f, b->Points()[f->points[0]]->pos, tex_w, tex_h, WHITE, aabb
                ));
				vertices.push_back(BrushBuilder::CreateVertex(
                    f, b->Points()[f->points[i]]->pos, tex_w, tex_h, WHITE, aabb
                ));
				vertices.push_back(BrushBuilder::CreateVertex(
                    f, b->Points()[f->points[i + 1]]->pos, tex_w, tex_h, WHITE, aabb
                ));
			}

			int start_idx = border_vertices.size();
			for (auto& v : f->points) {
				border_vertices.push_back(BrushBuilder::CreateVertex(
                    f, b->Points()[v]->pos, tex_w, tex_h, WHITE, aabb
                ));
			}
			for (int i = 0, n = f->points.size() - 1; i < n; ++i) {
				border_indices.push_back(start_idx + i);
				border_indices.push_back(start_idx + i + 1);
			}
			border_indices.push_back(start_idx + static_cast<unsigned short>(f->points.size() - 1));
			border_indices.push_back(start_idx);

			++face_idx;
		}

		if (!vertices.empty()) {
            BrushBuilder::FlushVertices(BrushBuilder::VertexType::PosNormTex,
                mesh, border_mesh, vertices, border_vertices, border_indices, dst);
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