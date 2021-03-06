#include "model/ObjLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <filesystem>

namespace
{

struct Vertex
{
	float pos[3];
	float normal[3];
	float texcoord[2];
};

}

namespace model
{

bool ObjLoader::Load(Model& model, const std::string& filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn, err;
	auto dir = std::filesystem::path(filepath).parent_path().string() + "/";
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str(), dir.c_str(), false);

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}



	//// Loop over shapes
	//for (size_t s = 0; s < shapes.size(); s++)
	//{
	//	std::vector<Vertex> vertices;
	//	std::vector<uint16_t> indices;

	//	// Loop over faces(polygon)
	//	size_t index_offset = 0;
	//	for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
	//		int fv = shapes[s].mesh.num_face_vertices[f];

	//		// Loop over vertices in the face.
	//		for (size_t v = 0; v < fv; v++) {
	//			// access to vertex
	//			tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
	//			tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
	//			tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
	//			tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
	//			tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
	//			tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
	//			tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
	//			tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
	//			tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
	//			// Optional: vertex colors
	//			// tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
	//			// tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
	//			// tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
	//		}
	//		index_offset += fv;

	//		// per-face material
	//		shapes[s].mesh.material_ids[f];
	//	}
	//}

	return true;
}

}
