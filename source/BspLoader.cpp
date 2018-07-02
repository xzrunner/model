#include "model/BspLoader.h"
#include "model/TextureLoader.h"

#include <quake/Palette.h>

#include <fstream>

#include <assert.h>

namespace model
{

bool BspLoader::Load(Model& model, const std::string& filepath)
{
	std::ifstream fin(filepath, std::ios::binary);
	if (fin.fail()) {
		return false;
	}

	BspHeader header;
	fin.read(reinterpret_cast<char*>(&header), sizeof(header));
	assert(header.version == BSPVERSION);
	//switch (header.version)
	//{
	//case BSPVERSION:
	//	break;
	//case BSP2VERSION_2PSB:
	//	break;
	//case BSP2VERSION_BSP2:
	//	break;
	//}

	std::vector<BspVertex> vertices;
	LoadVertices(fin, header.lumps[LUMP_VERTEXES], vertices);

	std::vector<BspEdge> edges;
	LoadEdges(fin, header.lumps[LUMP_EDGES], edges);

	std::vector<int> surface_edges;
	LoadSurfaceEdges(fin, header.lumps[LUMP_SURFEDGES], surface_edges);

	std::vector<std::unique_ptr<QuakeTexture>> textures;
	LoadTextures(fin, header.lumps[LUMP_TEXTURES], textures);

	return true;
}

void BspLoader::LoadVertices(std::ifstream& fin, const FileLump& lump,
	                         std::vector<BspVertex>& vertices)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspVertex);
	vertices.resize(n);
	fin.read(reinterpret_cast<char*>(vertices[0].point), lump.size);
}

void BspLoader::LoadEdges(std::ifstream& fin, const FileLump& lump,
	                      std::vector<BspEdge>& edges)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspEdge);
	edges.resize(n);
	fin.read(reinterpret_cast<char*>(edges[0].v), lump.size);
}

void BspLoader::LoadSurfaceEdges(std::ifstream& fin, const FileLump& lump,
	                             std::vector<int>& surface_edges)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(int);
	surface_edges.resize(n);
	fin.read(reinterpret_cast<char*>(&surface_edges[0]), lump.size);
}

void BspLoader::LoadTextures(std::ifstream& fin, const FileLump& lump,
	                         std::vector<std::unique_ptr<QuakeTexture>>& textures)
{
	BspMipTexLump* m = nullptr;

	int num_mip_tex = 0;
	if (lump.size != 0) {
		fin.seekg(lump.offset);
		fin.read(reinterpret_cast<char*>(&num_mip_tex), sizeof(int));
		m = reinterpret_cast<BspMipTexLump*>(new char[sizeof(BspMipTexLump) - sizeof(int) + sizeof(int) * num_mip_tex]);
		m->nummiptex = num_mip_tex;
		fin.read(reinterpret_cast<char*>(m->dataofs), sizeof(int) * num_mip_tex);
	}

	int num_textures = num_mip_tex + 2;
	textures.resize(num_textures);
	for (int i = 0; i < num_mip_tex; ++i)
	{
		if (m->dataofs[i] == -1) {
			textures[i] = nullptr;
			continue;
		}

		BspMipTex mt;
		fin.seekg(lump.offset + m->dataofs[i]);
		fin.read(reinterpret_cast<char*>(&mt), sizeof(mt));
		assert((mt.width & 15) == 0 && (mt.height & 15) == 0);
		auto tx = std::make_unique<QuakeTexture>();

		strcpy(tx->name, mt.name);
		//for (int j = 0; j < MIPLEVELS; ++j) {
		//	tx->offsets[j] = mt.offsets[j] - sizeof(BspMipTex) + sizeof(QuakeTexture);
		//}

		if (strncmp(tx->name, "sky", 3) == 0) {
			;	// todo sky texture
		} else if (tx->name[0] == '*') {
			;	// todo warping texture
		} else {
			// todo load texture from file

			int pixels_sz = mt.width * mt.height / 64 * 85;		// 4 / 3
			unsigned char* indexed = new unsigned char[pixels_sz];

			quake::Palette palette;
			unsigned char* rgb = new unsigned char[mt.width * mt.height * 3];
			palette.IndexedToRgb(indexed, mt.width * mt.height, rgb);
			delete[] indexed;

			tx->tex = TextureLoader::LoadFromMemory(rgb, mt.width, mt.height, 3);
			delete[] rgb;
		}

		textures[i] = std::move(tx);
	}
	textures[num_textures - 2] = std::make_unique<QuakeTexture>();
	textures[num_textures - 1] = std::make_unique<QuakeTexture>();

	delete[] m;

	// todo: sequence the animations
}

}