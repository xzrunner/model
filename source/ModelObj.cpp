#include "model/ModelObj.h"
#include "model/Surface.h"
#include "model/Mesh.h"
#include "model/typedef.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <assert.h>

namespace model 
{

const char* const ModelObj::TYPE_NAME = "object";

ModelObj::ModelObj()
	: m_scale(1)
{
}

//ModelObj::ModelObj(const std::string& filepath, float scale /*= 1.0f*/)
//	: m_scale(scale)
//{
////	Import(filepath);
//}

bool ModelObj::StoreToJson(rapidjson::Value& val, rapidjson::MemoryPoolAllocator<>& alloc) const
{
	return false;
}

void ModelObj::LoadFromJson(const rapidjson::Value& val)
{
}

void ModelObj::Import(const std::string& filepath)
{
	std::locale::global(std::locale(""));
	std::ifstream fin(filepath.c_str());
	std::locale::global(std::locale("C"));
	assert(fin);

	while (fin) {
		std::string line;
		getline(fin, line);
		if (line.empty()) continue;

		std::vector<std::string> tokens;
		SplitString(line, tokens);

		const std::string& tag = tokens[0];
		if (tag == "mtllib")
		{
			if (tokens.size() < 2) return Error(fin, filepath);

			std::string path(filepath);

			int a = path.rfind('/');
			int b = path.rfind('\\');
			std::string dir = path.substr(0, std::max(a, b) + 1);
			HandleMaterial((dir + tokens[1]).c_str());
		}
		else if (tag == "g")
		{
			if (tokens.size() < 2) return Error(fin, filepath);
			HandleObject();
			m_curr_object.name = tokens[1];
		}
		else if (tag == "usemtl")
		{
			if (tokens.size() < 2) return Error(fin, filepath);
			HandleMesh();
			m_curr_mesh.material_id = -1;
			for (size_t i = 0, n = m_materials.size(); i < n; ++i) {
				if (m_materials[i].name == tokens[1]) {
					m_curr_mesh.material_id = i;
					break;
				}
			}
		}
		else if (tag == "v")
		{
			if (tokens.size() < 4) return Error(fin, filepath);
			HandleVertice(tokens);
		}
		else if (tag == "f")
		{
			if (tokens.size() < 4) return Error(fin, filepath);
			HandleFace(tokens);
		}
	}
	HandleObject();

	fin.close();

	InitNormals();
	InitAllMeshes();
}

void ModelObj::InitNormals()
{
	m_normals.resize(m_vertices.size(), sm::vec3(0, 0, 0));
	for (size_t faceIndex = 0; faceIndex < m_faces.size(); ++faceIndex) {
		sm::ivec3 face = m_faces[faceIndex];

		// Compute the facet normal.
		sm::vec3 a = m_vertices[face.x];
		sm::vec3 b = m_vertices[face.y];
		sm::vec3 c = m_vertices[face.z];
		sm::vec3 facetNormal = (b - a).Cross(c - a);

		// Add the facet normal to the lighting normal of each adjoining vertex.
		m_normals[face.x] += facetNormal;
		m_normals[face.y] += facetNormal;
		m_normals[face.z] += facetNormal;
	}

	// Normalize the normals.
	for (size_t i = 0, n = m_normals.size(); i < n; ++i)
		m_normals[i].Normalize();

	// Copy to MeshInfo
	for (size_t i = 0, n = m_objects.size(); i < n; ++i) {
		for (size_t j = 0, m = m_objects[i].meshes.size(); j < m; ++j) {
			MeshInfo& mesh = m_objects[i].meshes[j];
			mesh.normals.resize(mesh.vertices.size(), sm::vec3());
			std::map<int, int>::iterator itr = mesh.map_index.begin();
			for ( ; itr != mesh.map_index.end(); ++itr)
				mesh.normals[itr->second] = m_normals[itr->first];
		}
	}
}

void ModelObj::InitAllMeshes()
{
	for (size_t i = 0, n = m_objects.size(); i < n; ++i) 
	{
		for (size_t j = 0, m = m_objects[i].meshes.size(); j < m; ++j) 
		{
			const MeshInfo& mesh_data = m_objects[i].meshes[j];
			const MaterialInfo& material_data = m_materials[mesh_data.material_id];

			auto mesh = std::make_shared<Mesh>();

			Material material;
			material.ambient  = material_data.ambient;
			material.diffuse  = material_data.diffuse;
			material.specular = material_data.specular;
			mesh->SetMaterial(material);

			int vertex_type = VERTEX_FLAG_NORMALS;
			int stride = 3 + 3;

			// Create the VBO for the vertices.
			std::vector<float> vertices;
			mesh_data.GenerateVertices(vertices, vertex_type);
			int vertex_count = mesh_data.GetVertexCount();

			// Create a new VBO for the indices if needed.
			int index_count = mesh_data.GetTriangleIndexCount();
			std::vector<unsigned short> indices;
			mesh_data.GenerateTriangleIndices(indices);

			mesh->SetRenderBuffer(vertex_type, vertices, indices);
			mesh->SetIndexCount(mesh_data.GetTriangleIndexCount());

			m_meshes.push_back(mesh);
		}
	}
}

void ModelObj::HandleMaterial(const std::string& filepath)
{
	std::locale::global(std::locale(""));
	std::ifstream fin(filepath.c_str());
	std::locale::global(std::locale("C"));
	assert(fin);

	MaterialInfo material;

	while (fin) {
		std::string line;
		getline(fin, line);
		if (line.empty()) continue;

		std::vector<std::string> tokens;
		SplitString(line, tokens);

		const std::string& tag = tokens[0];
		if (tag == "newmtl")
		{
			if (tokens.size() < 2) return Error(fin, filepath);

			if (!material.name.empty())
				m_materials.push_back(material);
			material.clear();
			material.name = tokens[1];
		}
		else if (tag == "Ka")
		{
			if (tokens.size() < 4) return Error(fin, filepath);
			material.ambient = String2Vec3(tokens[1], tokens[2], tokens[3]);
		}
		else if (tag == "Kd")
		{
			if (tokens.size() < 4) return Error(fin, filepath);
			material.diffuse = String2Vec3(tokens[1], tokens[2], tokens[3]);
		}
		else if (tag == "Ks")
		{
			if (tokens.size() < 4) return Error(fin, filepath);
			material.specular = String2Vec3(tokens[1], tokens[2], tokens[3]);
		}
	}

	if (!material.name.empty())
		m_materials.push_back(material);

	fin.close();
}

void ModelObj::HandleObject()
{
	HandleMesh();
	if (!m_curr_object.meshes.empty())
	{
		m_objects.push_back(m_curr_object);
		m_curr_object.clear();
	}
}

void ModelObj::HandleMesh()
{
	if (!m_curr_mesh.faces.empty())
	{
		m_curr_object.meshes.push_back(m_curr_mesh);
		m_curr_mesh.clear();
	}
}

void ModelObj::HandleVertice(const std::vector<std::string>& tokens)
{
	sm::vec3 pos = String2Vec3(tokens[1], tokens[2], tokens[3]);
	m_vertices.push_back(pos * m_scale);
}

void ModelObj::HandleTextureCoordinate(const std::vector<std::string>& tokens)
{
	
}

void ModelObj::HandleNormal(const std::vector<std::string>& tokens)
{
	
}

void ModelObj::HandleFace(const std::vector<std::string>& tokens)
{
	// split
	if (tokens.size() > 4)
	{
		size_t size = tokens.size()-1;
		std::vector<uint16_t> nodes;
		nodes.resize(size);
		for (size_t i = 0; i < size; ++i)
		{
			std::vector<std::string> elems;
			SplitString(tokens[1+i], elems, '/');
			nodes[i] = atoi(elems[0].c_str());
		}

//		if (size % 2 == 0) {
//// 			for (size_t i = 0, n = size/2-1; i < n; ++i)
//// 			{
//// 				int index = i * 2;
//// 
//// 				_curr_mesh.AddFace(*this, nodes[index], nodes[index+1], nodes[index+2]);
//// 				_curr_mesh.AddFace(*this, nodes[index], nodes[index+2], nodes[index+3]);
//// 
//// 				if (i == 0) break;
//// 			}
//
//			// fan
// 			for (size_t i = 2; i < size; ++i)
// 			{
// 				_curr_mesh.AddFace(*this, nodes[0], nodes[i-1], nodes[i]);
// 			}
//
//		} else {
 			for (size_t i = 2; i < size; ++i)
 				m_curr_mesh.AddFace(*this, nodes[0], nodes[i-1], nodes[i]);
//		}
	}
	else
	{
		uint16_t nodes[3];
		for (size_t i = 0; i < 3; ++i)
		{
			std::vector<std::string> elems;
			SplitString(tokens[1+i], elems, '/');
			nodes[i] = atoi(elems[0].c_str());
		}

		m_curr_mesh.AddFace(*this, nodes[0], nodes[1], nodes[2]);
	}
}

void ModelObj::Error(std::ifstream& fin, const std::string& filepath)
{
	fin.close();
}

void ModelObj::SplitString(const std::string& src, std::vector<std::string>& dst)
{
	dst.clear();
	std::istringstream iss(src);
	copy(std::istream_iterator<std::string>(iss),
		std::istream_iterator<std::string>(),
		std::back_inserter<std::vector<std::string> >(dst));
}

void ModelObj::SplitString(const std::string& src, std::vector<std::string>& dst, char delim)
{
	std::stringstream ss(src);
	std::string item;
	while (std::getline(ss, item, delim)) {
		dst.push_back(item);
	}
}

sm::vec3 ModelObj::String2Vec3(const std::string& sx, const std::string& sy, const std::string& sz)
{
	sm::vec3 ret;
	ret.x = static_cast<float>(atof(sx.c_str()));
	ret.y = static_cast<float>(atof(sy.c_str()));
	ret.z = static_cast<float>(atof(sz.c_str()));
	return ret;
}

//////////////////////////////////////////////////////////////////////////
// class ModelObj::MeshInfo
//////////////////////////////////////////////////////////////////////////

int ModelObj::MeshInfo::
GetVertexCount() const
{
	return vertices.size();
}

int ModelObj::MeshInfo::
GetTriangleIndexCount() const
{
	return faces.size() * 3;
}

void ModelObj::MeshInfo::
GenerateVertices(std::vector<float>& floats, unsigned char flags) const
{
	struct Vertex {
		sm::vec3 Position;
		sm::vec3 Normal;
	};

	// Read in the vertex positions and initialize lighting normals to (0, 0, 0).
	floats.resize(GetVertexCount() * 6);
	Vertex* vertex = (Vertex*) &floats[0];
	for (size_t i = 0, n = vertices.size(); i < n; ++i, ++vertex)
	{
		vertex->Position = vertices[i];
		vertex->Normal = normals[i];
	}
}

void ModelObj::MeshInfo::
GenerateTriangleIndices(std::vector<unsigned short>& indices) const
{
	indices.resize(GetTriangleIndexCount());
	std::vector<unsigned short>::iterator index = indices.begin();
	for (std::vector<sm::ivec3>::const_iterator f = faces.begin(); f != faces.end(); ++f) {
		*index++ = f->x;
		*index++ = f->y;
		*index++ = f->z;
	}
}

void ModelObj::MeshInfo::
AddFace(ModelObj& obj, int i0, int i1, int i2)
{
	i0 -= 1;
	i1 -= 1;
	i2 -= 1;

	obj.m_faces.push_back(sm::ivec3(i0, i1, i2));
	i0 = AddNode(obj, i0);
	i1 = AddNode(obj, i1);
	i2 = AddNode(obj, i2);
	faces.push_back(sm::ivec3(i0, i1, i2));
}

int ModelObj::MeshInfo::
AddNode(ModelObj& obj, int index)
{
	std::map<int, int>::iterator itr = map_index.find(index);
	if (itr != map_index.end()) {
		return itr->second;
	} else {
		vertices.push_back(obj.m_vertices[index]);
		int ret = map_index.size();
		map_index.insert(std::make_pair(index, ret));
		return ret;
	}
}

}	