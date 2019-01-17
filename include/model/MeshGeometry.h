#pragma once

#include <SM_Matrix.h>
#include <unirender/VertexAttrib.h>

#include <unordered_map>
#include <memory>

#include <boost/noncopyable.hpp>

namespace model
{

struct SubmeshGeometry
{
	SubmeshGeometry() {}
	SubmeshGeometry(bool index, size_t count, size_t offset)
		: index(index), count(count), offset(offset) {}

	bool index = true;
	size_t count = 0;
	size_t offset = 0;
};

struct Bone
{
	int node = -1;
	std::string name;
	sm::mat4 offset_trans;
};

struct MeshRawData
{
	std::vector<sm::vec3>         vertices;
	std::vector<sm::vec3>         normals;
	std::vector<std::vector<int>> faces;

	void CalcNormals();
};

struct BlendShapeData
{
    std::string name;
    std::vector<sm::vec3> vertices;
    //// todo gpu
    //unsigned int vbo = 0;
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

	// skeletal anim
	std::vector<Bone> bones;

	// morph anim
	std::vector<ur::VertexAttrib> vertex_layout;

	// mesh process
	std::unique_ptr<MeshRawData> raw_data = nullptr;

    // blend shape
    size_t n_vert = 0, n_poly = 0;
    uint8_t* vert_buf = nullptr;
    size_t vert_stride = 0;
    std::vector<std::unique_ptr<BlendShapeData>> blendshape_data;

	unsigned int vertex_type = 0;

}; // MeshGeometry

}