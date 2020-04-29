#pragma once

#include <SM_Matrix.h>

#include <unordered_map>
#include <memory>

#include <boost/noncopyable.hpp>

//#define BLENDSHAPE_COMPRESS_FLOAT
//#define BLENDSHAPE_COMPRESS_TO8

namespace ur { class VertexArray; }

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
    std::vector<sm::vec2>         texcoords;
	std::vector<std::vector<int>> faces;
    std::vector<std::vector<std::pair<int, float>>> weights_per_vertex;

	void CalcNormals();
};

struct BlendShapeData
{
    std::string name;

    // compressed
#ifdef BLENDSHAPE_COMPRESS_FLOAT
    float flt_min, flt_max;
#ifdef BLENDSHAPE_COMPRESS_TO8
    std::vector<uint8_t> off_verts_idx;
#else
    std::vector<uint16_t> off_verts_idx;
#endif // BLENDSHAPE_COMPRESS_TO8
#else
    std::vector<sm::vec3> off_verts;
#endif // BLENDSHAPE_COMPRESS_FLOAT
    std::vector<uint32_t> idx_verts;

    //// todo gpu
    //unsigned int vbo = 0;

    void SetVertices(const std::vector<sm::vec3>& ori_verts,
        const std::vector<sm::vec3>& new_verts);

};

struct MeshGeometry : boost::noncopyable
{
	~MeshGeometry();

    std::shared_ptr<ur::VertexArray> vertex_array = nullptr;

//	std::unordered_map<std::string, SubmeshGeometry> sub_geometries;
	std::vector<SubmeshGeometry> sub_geometries;
	std::vector<unsigned int>    sub_geometry_materials;

	// skeletal anim
	std::vector<Bone> bones;

	//// morph anim
	//std::vector<ur::VertexAttrib> vertex_layout;

	// mesh process
	std::unique_ptr<MeshRawData> raw_data = nullptr;

    // blend shape
    size_t n_vert = 0, n_poly = 0;
    const uint8_t* vert_buf = nullptr;
    size_t vert_stride = 0;
    std::vector<std::unique_ptr<BlendShapeData>> blendshape_data;

	unsigned int vertex_type = 0;

}; // MeshGeometry

}