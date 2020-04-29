#pragma once

#include "model/BspFile.h"
#include "model/BspModel.h"

#include <string>
#include <vector>
#include <memory>

namespace ur { class VertexArray; class Device; }

namespace model
{

struct Model;

class BspLoader
{
public:
	static bool Load(const ur::Device& dev, Model& model, const std::string& filepath);

private:
	static void LoadVertices(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspVertex>& vertices);
	static void LoadEdges(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspEdge>& edges);
	static void LoadSurfaceEdges(std::ifstream& fin, const BspFileLump& lump,
		std::vector<int>& surface_edges);
	static void LoadTextures(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspModel::Texture>& textures, const ur::Device& dev);
	static uint8_t* LoadLighting(std::ifstream& fin, const BspFileLump& lump);
	static void LoadPlanes(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspModel::Plane>& planes);
	static void LoadTexInfo(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspModel::TexInfo>& info, const std::vector<BspModel::Texture>& textures);
	static void LoadFaces(std::ifstream& fin, const BspFileLump& lump, BspModel& model);
	static void LoadMarkSurfaces(std::ifstream& fin, const BspFileLump& lump, BspModel& model);
	static uint8_t* LoadVisibility(std::ifstream& fin, const BspFileLump& lump);
	static void LoadLeafs(std::ifstream& fin, const BspFileLump& lump, BspModel& model);
	static void LoadNodes(std::ifstream& fin, const BspFileLump& lump, BspModel& model);
	static void LoadClipnodes(std::ifstream& fin, const BspFileLump& lump, BspModel& model);
	static int8_t* LoadEntities(std::ifstream& fin, const BspFileLump& lump);
	static void LoadSubmodels(std::ifstream& fin, const BspFileLump& lump,
		std::vector<BspSubmodel>& submodels);

	static void CalcSurfaceExtents(BspModel::Surface& s, const BspModel& model);
	static void CalcSurfaceBounds(BspModel::Surface& s, const BspModel& model);

	static void ChainSurfaceByTexture(BspModel& model);

	static void BuildModelVertexBuffer(const ur::Device& dev,
        const BspModel& model, ur::VertexArray& va);
	static void BuildModelIndexBuffer(const ur::Device& dev,
        const BspModel& model, ur::VertexArray& va);

}; // BspLoader

}