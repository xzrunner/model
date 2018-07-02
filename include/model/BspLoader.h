#pragma once

#include "model/BspFile.h"
#include "model/QuakeModel.h"

#include <string>
#include <vector>
#include <memory>

namespace model
{

struct Model;

class BspLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	static void LoadVertices(std::ifstream& fin, const FileLump& lump,
		std::vector<BspVertex>& vertices);
	static void LoadEdges(std::ifstream& fin, const FileLump& lump,
		std::vector<BspEdge>& edges);
	static void LoadSurfaceEdges(std::ifstream& fin, const FileLump& lump,
		std::vector<int>& surface_edges);
	static void LoadTextures(std::ifstream& fin, const FileLump& lump,
		std::vector<std::unique_ptr<QuakeTexture>>& textures);

}; // BspLoader

}