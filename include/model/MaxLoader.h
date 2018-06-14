#pragma once

#include <rapidxml.hpp>

#include <string>
#include <vector>

namespace model
{

struct Model;

class MaxLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	struct MapChannel
	{
		std::string name;

		std::vector<float> vertices;
		std::vector<int>   faces;
	};

	struct MeshData
	{
		std::vector<float> vertices;
		std::vector<float> normals;

		int face_count;

		std::vector<int> face_vertices;
		std::vector<int> face_normals;
		std::vector<int> face_material_ids;
		std::vector<int> face_smoothing_groups;
		std::vector<int> face_edge_visibility;

		std::vector<MapChannel> map_channels;
	};

private:
	static void LoadMesh(Model& model, const rapidxml::xml_node<>* mesh_node);

	static void LoadMeshData(MeshData& dst, const rapidxml::xml_node<>* src);

	template<typename T>
	static void ParseArray(std::vector<T>& dst, const rapidxml::xml_node<>* src);

}; // MaxLoader

}

#include "model/MaxLoader.inl"