#include "model/MaxLoader.h"
#include "model/Model.h"
#include "model/typedef.h"

#include <SM_Vector.h>
#include <unirender2/Device.h>
#include <unirender2/VertexArray.h>
#include <unirender2/IndexBuffer.h>
#include <unirender2/VertexBuffer.h>
#include <unirender2/VertexBufferAttribute.h>

#include <rapidxml_utils.hpp>

#include <boost/algorithm/string.hpp>

namespace
{

struct Vertex
{
	sm::vec3 pos;
	sm::vec3 normal;
	sm::vec2 texcoord;
};

bool coordinate_system_dx = false;

}

namespace model
{

bool MaxLoader::Load(const ur2::Device& dev, Model& model, const std::string& filepath)
{
	rapidxml::file<> xml_file(filepath.c_str());
	rapidxml::xml_document<> doc;
	doc.parse<0>(xml_file.data());

	auto info = doc.first_node()->first_node("SceneInfo");
	auto coordinate_system = info->first_attribute("CoordinateSystem");
	if (coordinate_system) {
		coordinate_system_dx = (strcmp(coordinate_system->value(), "directx") == 0);
	}

	auto node = doc.first_node()->first_node("Node");
	auto node_type = node->first_attribute("NodeType");
	if (strcmp(node_type->value(), "Mesh") == 0)
	{
		auto mesh_info = node->first_node("Mesh");
		LoadMesh(dev, model, mesh_info);
	}

	return true;
}

void MaxLoader::LoadMesh(const ur2::Device& dev, Model& model,
                         const rapidxml::xml_node<>* mesh_node)
{
	MeshData data;
	LoadMeshData(data, mesh_node);

	assert(data.map_channels.size() > 0);

	std::vector<Vertex> vertices;
	vertices.resize(data.face_count * 3);
	for (int i = 0, n = data.face_count; i < n; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			int src_idx = i * 3 + j;
			int dst_idx = coordinate_system_dx ? i * 3 + 2 - j : i * 3 + j;

			auto& src = data.map_channels[0];
			auto& dst = vertices[dst_idx];
			// texcoord
			int uv_idx = src.faces[src_idx];
			float* uv_ptr = &src.vertices[uv_idx * 3];
			dst.texcoord.x = uv_ptr[0];
			dst.texcoord.y = uv_ptr[1];
			// pos
			float* pos_ptr = &data.vertices[data.face_vertices[src_idx] * 3];
			dst.pos.x = pos_ptr[0];
			dst.pos.y = pos_ptr[1];
			dst.pos.z = pos_ptr[2];

			// normal
			float* normal_ptr = &data.normals[data.face_normals[src_idx] * 3];
			dst.normal.x = normal_ptr[0];
			dst.normal.y = normal_ptr[1];
			dst.normal.z = normal_ptr[2];
		}
	}

	const int stride = sizeof(Vertex) / sizeof(float);


    auto va = dev.CreateVertexArray();

    auto vbuf_sz = sizeof(Vertex) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur2::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(&vertices[0].pos.x, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur2::VertexBufferAttribute>> vbuf_attrs(3);
    // pos
    vbuf_attrs[0] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 0, 32);
    // normal
    vbuf_attrs[1] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 12, 32);
    // texcoord
    vbuf_attrs[2] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 2, 24, 32);
    va->SetVertexBufferAttrs(vbuf_attrs);

	// material
	model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
    mesh->geometry.vertex_array = va;
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, vertices.size(), 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	model.meshes.push_back(std::move(mesh));
}

void MaxLoader::LoadMeshData(MeshData& dst, const rapidxml::xml_node<>* src)
{
	// vertices
	auto vertices_node = src->first_node("Vertices");
	dst.vertices.resize(3 * std::atoi(vertices_node->first_attribute("Count")->value()));
	ParseArray(dst.vertices, vertices_node);

	// normals
	auto normals_node = src->first_node("Normals");
	dst.normals.resize(3 * std::atoi(normals_node->first_attribute("Count")->value()));
	ParseArray(dst.normals, normals_node);

	// faces
	auto faces_node = src->first_node("Faces");
	dst.face_count = std::atoi(faces_node->first_attribute("Count")->value());
	// face vertices
	dst.face_vertices.resize(3 * dst.face_count);
	ParseArray(dst.face_vertices, faces_node->first_node("FaceVertices"));
	// face normals
	dst.face_normals.resize(3 * dst.face_count);
	ParseArray(dst.face_normals, faces_node->first_node("FaceNormals"));
	// face material ids
	dst.face_material_ids.resize(dst.face_count);
	ParseArray(dst.face_material_ids, faces_node->first_node("MaterialIDs"));
	// face smoothing groups
	dst.face_smoothing_groups.resize(dst.face_count);
	ParseArray(dst.face_smoothing_groups, faces_node->first_node("SmoothingGroups"));
	// face edge visibility
	dst.face_edge_visibility.resize(dst.face_count * 3);
	ParseArray(dst.face_edge_visibility, faces_node->first_node("EdgeVisibility"));

	// map channels
	auto map_channels = src->first_node("MapChannels");
	int map_channels_n = std::atoi(map_channels->first_attribute("Count")->value());
	dst.map_channels.resize(map_channels_n);
	int idx = 0;
	for (auto channel_node = map_channels->first_node("MapChannel"); channel_node; channel_node = channel_node->next_sibling(), ++idx)
	{
		auto& dst_channel = dst.map_channels[idx];
		// vertices
		auto vertices_node = channel_node->first_node("MapVertices");
		dst_channel.vertices.resize(3 * std::atoi(vertices_node->first_attribute("Count")->value()));
		ParseArray(dst_channel.vertices, vertices_node);
		// faces
		auto faces_node = channel_node->first_node("MapFaces");
		dst_channel.faces.resize(3 * std::atoi(faces_node->first_attribute("Count")->value()));
		ParseArray(dst_channel.faces, faces_node);
	}
}

}