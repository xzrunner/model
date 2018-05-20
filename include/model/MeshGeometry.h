#pragma once

#include <SM_Matrix.h>

#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace model
{

struct SubmeshGeometry
{
	SubmeshGeometry() {}
	SubmeshGeometry(size_t index_count, size_t index_offset)
		: index_count(index_count), index_offset(index_offset) {}

	size_t index_count = 0;
	size_t index_offset = 0;
};

struct Bone
{
	int node = -1;
	std::string name;
	sm::mat4 offset_trans;
};

struct MeshGeometry : boost::noncopyable
{
	~MeshGeometry();

	unsigned int vao = 0;
	unsigned int vbo = 0;
	unsigned int ebo = 0;

//	std::unordered_map<std::string, SubmeshGeometry> sub_geometries;
	std::vector<SubmeshGeometry> sub_geometries;
	std::vector<unsigned int>    sub_geometry_materials;

	std::vector<Bone> bones;

	unsigned int vertex_type = 0;

}; // MeshGeometry

}