// code from http://tfc.duke.free.fr/coding/mdl-specs-en.html

#include "model/MdlLoader.h"
#include "model/Callback.h"
#include "model/Model.h"
#include "model/EffectType.h"
#include "model/NormalMap.h"
#include "model/typedef.h"

#include <quake/Palette.h>
#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>
#include <gimg_import.h>
#include <gimg_typedef.h>

#include <fstream>
#include <memory>

namespace
{

struct Vertex
{
	sm::vec3 pos;
	sm::vec3 normal;
	sm::vec2 texcoord;
};

const float SCALE = 0.1f;

}

namespace model
{

bool MdlLoader::Load(Model& model, const std::string& filepath)
{
	std::ifstream fin(filepath, std::ios::binary);
	if (fin.fail()) {
		return false;
	}

	MdlHeader header;
	fin.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (strncmp(header.magic, "IDPO", 4) != 0 || header.version != 6) {
		fin.close();
		return false;
	}

	LoadMaterial(header, fin, model, filepath);
	LoadMesh(header, fin, model);

	return true;
}

void MdlLoader::LoadMaterial(const MdlHeader& header, std::ifstream& fin,
	                         Model& model, const std::string& filepath)
{
	quake::Palette palette;

	MdlSkin* skins = new MdlSkin[header.num_skins];
	int skin_sz = header.skinwidth * header.skinheight;
	for (int i = 0; i < header.num_skins; ++i)
	{
		auto& skin = skins[i];
		skin.data = new char[skin_sz];
		fin.read(reinterpret_cast<char*>(&skin.group), sizeof(int));
		fin.read(skin.data, skin_sz);

		unsigned char* rgb = new unsigned char[skin_sz * 3];
		palette.IndexedToRgb((unsigned char*)skin.data, skin_sz, rgb);
		delete[] skin.data;

		gimg_revert_y(rgb, header.skinwidth, header.skinheight, GPF_RGB);

		auto material = std::make_unique<Model::Material>();
		material->diffuse_tex = model.textures.size();
		model.textures.push_back({ filepath + std::to_string(i),
			Callback::CreateImg(rgb, header.skinwidth, header.skinheight, 3) });
		model.materials.push_back(std::move(material));
	}
}

void MdlLoader::LoadMesh(const MdlHeader& header, std::ifstream& fin, Model& model)
{
	MdlTexcoord* texcoords = new MdlTexcoord[header.num_verts];
	fin.read(reinterpret_cast<char*>(texcoords), sizeof(MdlTexcoord) * header.num_verts);

	MdlTriangle* tris = new MdlTriangle[header.num_tris];
	fin.read(reinterpret_cast<char*>(tris), sizeof(MdlTriangle) * header.num_tris);

	pt3::AABB aabb;

	std::vector<sm::vec3> positions;
	positions.resize(header.num_verts);

	std::vector<Vertex> vertices;
	vertices.resize(header.num_frames * header.num_tris * 3);
	int v_ptr = 0;

	MdlFrame* frames = new MdlFrame[header.num_frames];
	for (int i = 0; i < header.num_frames; ++i)
	{
		auto& frame = frames[i];
		frame.frame.verts = new MdlVertex[header.num_verts];
		fin.read(reinterpret_cast<char*>(&frame.type), sizeof(int));
		assert(frame.type == 0);
		fin.read(reinterpret_cast<char*>(&frame.frame.bboxmin), sizeof(MdlVertex));
		fin.read(reinterpret_cast<char*>(&frame.frame.bboxmax), sizeof(MdlVertex));
		fin.read(reinterpret_cast<char*>(frame.frame.name), 16);
		fin.read(reinterpret_cast<char*>(frame.frame.verts), sizeof(MdlVertex) * header.num_verts);

		for (int j = 0; j < header.num_verts; ++j)
		{
			auto pos = TransVertex(frame.frame.verts[j], header.scale, header.translate) * SCALE;
			positions[j] = pos;
			aabb.Combine(pos);
		}
		for (int j = 0; j < header.num_tris; ++j)
		{
			auto& tri = tris[j];
			for (int k = 0; k < 3; ++k)
			{
				int vert_idx = tri.vertex[k];

				vertices[v_ptr].pos = positions[vert_idx];

				auto& texcoord = texcoords[vert_idx];
				float s = static_cast<float>(texcoord.s);
				float t = static_cast<float>(texcoord.t);
				if (!tri.facesfront && texcoords->onseam) {
					s += header.skinwidth * 0.5f;
				}
				s = (s + 0.5f) / header.skinwidth;
				t = (t + 0.5f) / header.skinheight;
				vertices[v_ptr].texcoord.Set(s, t);

				vertices[v_ptr].normal = NORMAL_MAP[frame.frame.verts[j].normal_idx];

				++v_ptr;
			}
		}
	}

	const int stride = sizeof(Vertex) / sizeof(float);

	ur::RenderContext::VertexInfo vi;

	vi.vn = vertices.size();
	vi.vertices = &vertices[0].pos.x;
	vi.stride = sizeof(Vertex);

	vi.in = 0;
	vi.indices = nullptr;

	vi.va_list.push_back(ur::RenderContext::VertexAttribute(3, 4));  // pos
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(3, 4));  // normal
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(2, 4));  // texcoord

	// material
	model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, vi.vn, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	mesh->effect = EFFECT_MORPH_TARGET;
	model.meshes.push_back(std::move(mesh));

	model.aabb = aabb;
}

sm::vec3 MdlLoader::TransVertex(const MdlVertex& vertex, const sm::vec3& scale, const sm::vec3& translate)
{
	sm::vec3 ret;
	for (int i = 0; i < 3; ++i) {
		ret.xyz[i] = vertex.v[i] * scale.xyz[i] + translate.xyz[i];
	}
	return ret;
}

}