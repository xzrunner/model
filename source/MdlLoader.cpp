// code from http://tfc.duke.free.fr/coding/mdl-specs-en.html

#include "model/MdlLoader.h"
#include "model/Callback.h"
#include "model/Model.h"
#include "model/EffectType.h"
#include "model/NormalMap.h"
#include "model/typedef.h"
#include "model/MorphTargetAnim.h"

#include <quake/Palette.h>
#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>
#include <SM_Cube.h>

#include <fstream>
#include <memory>

namespace
{

struct Vertex
{
	sm::vec3 pos;
	sm::vec3 normal;
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

		auto material = std::make_unique<Model::Material>();
		material->diffuse_tex = model.textures.size();
		model.textures.push_back({ filepath + std::to_string(i),
			Callback::CreateImg(rgb, header.skinwidth, header.skinheight, 3) });
		model.materials.push_back(std::move(material));

		delete[] rgb;
	}
	delete[] skins;
}

void MdlLoader::LoadMesh(const MdlHeader& header, std::ifstream& fin, Model& model)
{
	MdlTexcoord* mdl_texcoords = new MdlTexcoord[header.num_verts];
	fin.read(reinterpret_cast<char*>(mdl_texcoords), sizeof(MdlTexcoord) * header.num_verts);

	MdlTriangle* mdl_tris = new MdlTriangle[header.num_tris];
	fin.read(reinterpret_cast<char*>(mdl_tris), sizeof(MdlTriangle) * header.num_tris);

	sm::cube aabb;

	std::vector<sm::vec3> positions;
	positions.resize(header.num_verts);

	std::vector<Vertex> vertices;
	vertices.resize(header.num_frames * header.num_tris * 3);
	int v_ptr = 0;

	MdlFrame* mdl_frames = new MdlFrame[header.num_frames];
	for (int i = 0; i < header.num_frames; ++i)
	{
		auto& frame = mdl_frames[i];
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
			auto& tri = mdl_tris[j];
			for (int k = 0; k < 3; ++k)
			{
				int vert_idx = tri.vertex[k];
				vertices[v_ptr].pos = positions[vert_idx];
				vertices[v_ptr].normal = NORMAL_MAP[frame.frame.verts[j].normal_idx];
				++v_ptr;
			}
		}

		delete[] frame.frame.verts;
		frame.frame.verts = nullptr;
	}

	std::vector<sm::vec2> texcoords;
	texcoords.resize(header.num_tris * 3);
	v_ptr = 0;
	for (int i = 0; i < header.num_tris; ++i)
	{
		auto& tri = mdl_tris[i];
		for (int j = 0; j < 3; ++j)
		{
			int vert_idx = tri.vertex[j];

			auto& texcoord = mdl_texcoords[vert_idx];
			float s = static_cast<float>(texcoord.s);
			float t = static_cast<float>(texcoord.t);
			if (!tri.facesfront && texcoord.onseam) {
				s += header.skinwidth * 0.5f;
			}
			s = (s + 0.5f) / header.skinwidth;
			t = (t + 0.5f) / header.skinheight;

			texcoords[v_ptr].Set(s, t);

			++v_ptr;
		}
	}

	delete[] mdl_frames;
	delete[] mdl_tris;
	delete[] mdl_texcoords;

	// material
	model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();

	int vt_sz = sizeof(Vertex) * vertices.size();
	int tc_sz = sizeof(sm::vec2) * texcoords.size();
	uint8_t* buf = new uint8_t[vt_sz + tc_sz];
	memcpy(buf, vertices.data(), vt_sz);
	memcpy(buf + vt_sz, texcoords.data(), tc_sz);
	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
	mesh->geometry.vbo = rc.CreateBuffer(ur::VERTEXBUFFER, buf, vt_sz + tc_sz);
	delete[] buf;

	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("pose1_vert",   3, 4, 24, 0));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("pose1_normal", 3, 4, 24, 12));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("pose2_vert",   3, 4, 24, 0));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("pose2_normal", 3, 4, 24, 12));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("texcoord",     2, 4, 0, vt_sz));

	int vertices_n = header.num_tris * 3;
	int offset = 0;
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, vertices_n, 0));
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	mesh->effect = EFFECT_MORPH_TARGET;
	model.meshes.push_back(std::move(mesh));

	model.anim = std::make_unique<MorphTargetAnim>(3, header.num_frames, header.num_tris * 3);

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