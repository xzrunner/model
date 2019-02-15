#include "model/BspLoader.h"
#include "model/TextureLoader.h"
#include "model/BspModel.h"
#include "model/Model.h"
#include "model/typedef.h"

#include <unirender/Blackboard.h>
#include <unirender/RenderContext.h>
#include <unirender/typedef.h>
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

	auto bsp = std::make_unique<BspModel>();

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

	LoadVertices(fin, header.lumps[LUMP_VERTEXES], bsp->vertices);
	LoadEdges(fin, header.lumps[LUMP_EDGES], bsp->edges);
	LoadSurfaceEdges(fin, header.lumps[LUMP_SURFEDGES], bsp->surface_edges);
	LoadTextures(fin, header.lumps[LUMP_TEXTURES], bsp->textures);
	bsp->lightdata = LoadLighting(fin, header.lumps[LUMP_LIGHTING]);
	LoadPlanes(fin, header.lumps[LUMP_PLANES], bsp->planes);
	LoadTexInfo(fin, header.lumps[LUMP_TEXINFO], bsp->tex_info, bsp->textures);
	LoadFaces(fin, header.lumps[LUMP_FACES], *bsp);
	LoadMarkSurfaces(fin, header.lumps[LUMP_MARKSURFACES], *bsp);
	bsp->visdata = LoadVisibility(fin, header.lumps[LUMP_VISIBILITY]);
	LoadLeafs(fin, header.lumps[LUMP_LEAFS], *bsp);
	LoadNodes(fin, header.lumps[LUMP_NODES], *bsp);
	bsp->entities = LoadEntities(fin, header.lumps[LUMP_ENTITIES]);
	LoadSubmodels(fin, header.lumps[LUMP_MODELS], bsp->submodels);

	bsp->CreateSurfaceLightmap();
	bsp->BuildSurfaceDisplayList();

	ChainSurfaceByTexture(*bsp);

	// material
	model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
	mesh->geometry.vbo = BuildModelVertexBuffer(*bsp);
	mesh->geometry.ebo = BuildModelIndexBuffer(*bsp);

	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("position",       3, 4, 28, 0));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("texcoord",       2, 4, 28, 12));
	mesh->geometry.vertex_layout.push_back(ur::VertexAttrib("texcoord_light", 2, 4, 28, 20));

	int vertices_n = 0;
	for (auto& s : bsp->surfaces) {
		vertices_n += 3 * (s.numedges - 2);
	}
	int offset = 0;
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, vertices_n, 0));
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	model.meshes.push_back(std::move(mesh));

//	model.aabb = aabb;

	model.ext = std::move(bsp);

	return true;
}

void BspLoader::LoadVertices(std::ifstream& fin, const BspFileLump& lump,
	                         std::vector<BspVertex>& vertices)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspVertex);
	vertices.resize(n);
	fin.read(reinterpret_cast<char*>(vertices[0].point), lump.size);
}

void BspLoader::LoadEdges(std::ifstream& fin, const BspFileLump& lump,
	                      std::vector<BspEdge>& edges)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspEdge);
	edges.resize(n);
	fin.read(reinterpret_cast<char*>(edges[0].v), lump.size);
}

void BspLoader::LoadSurfaceEdges(std::ifstream& fin, const BspFileLump& lump,
	                             std::vector<int>& surface_edges)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(int);
	surface_edges.resize(n);
	fin.read(reinterpret_cast<char*>(&surface_edges[0]), lump.size);
}

void BspLoader::LoadTextures(std::ifstream& fin, const BspFileLump& lump,
	                         std::vector<BspModel::Texture>& textures)
{
	BspMipTexLump* m = nullptr;

	quake::Palette palette;
	auto& rc = ur::Blackboard::Instance()->GetRenderContext();

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
			textures[i].tex = nullptr;
			textures[i].surfaces_chain = nullptr;
			continue;
		}

		BspMipTex mt;
		fin.seekg(lump.offset + m->dataofs[i]);
		fin.read(reinterpret_cast<char*>(&mt), sizeof(mt));
		assert((mt.width & 15) == 0 && (mt.height & 15) == 0);

		if (strncmp(mt.name, "sky", 3) == 0) {
			;	// todo sky texture
		} else if (mt.name[0] == '*') {
			;	// todo warping texture
		} else {
			// todo load texture from file

			int tex_id = rc.CreateTextureID(mt.width, mt.height, ur::TEXTURE_RGB, MIPLEVELS);
			uint32_t mip_w = mt.width;
			uint32_t mip_h = mt.height;
			for (int j = 0; j < MIPLEVELS; ++j)
			{
				size_t pixel_sz = mip_w * mip_h;
				unsigned char* indexed = new unsigned char[pixel_sz];
				fin.seekg(lump.offset + m->dataofs[i] + mt.offsets[j]);
				fin.read((char*)indexed, pixel_sz);

				size_t rgb_sz = pixel_sz * 3;
				unsigned char* rgb = new unsigned char[rgb_sz];
				palette.IndexedToRgb(indexed, pixel_sz, rgb);
				delete[] indexed;

				rc.UpdateTexture(tex_id, rgb, mip_w, mip_h, 0, j, ur::TEXTURE_WARP_REPEAT);
				delete[] rgb;

				mip_w /= 2;
				mip_h /= 2;
			}
			textures[i].tex = std::make_unique<ur::Texture>(&rc, mt.width, mt.height, ur::TEXTURE_RGB, tex_id);
			textures[i].surfaces_chain = nullptr;
		}
	}
	textures[num_textures - 2].tex = std::make_unique<ur::Texture>();
	textures[num_textures - 2].surfaces_chain = nullptr;
	textures[num_textures - 1].tex = std::make_unique<ur::Texture>();
	textures[num_textures - 1].surfaces_chain = nullptr;

	delete[] m;

	// todo: sequence the animations
}

uint8_t* BspLoader::LoadLighting(std::ifstream& fin, const BspFileLump& lump)
{
	if (!lump.size) {
		return nullptr;
	}

	uint8_t* lighting = new uint8_t[lump.size * 3];
	auto in = lighting + lump.size * 2;
	auto out = lighting;
	fin.seekg(lump.offset);
	fin.read((char*)in, lump.size);
	for (int i = 0; i < lump.size; ++i)
	{
		auto d = *in++;
		*out++ = d;
		*out++ = d;
		*out++ = d;
	}

	return lighting;
}

void BspLoader::LoadPlanes(std::ifstream& fin, const BspFileLump& lump,
	                       std::vector<BspModel::Plane>& planes)
{
	int n = lump.size / sizeof(BspPlane);

	std::vector<BspPlane> src_planes;
	src_planes.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_planes[0]), lump.size);

	planes.resize(n);
	for (int i = 0; i < n; ++i)
	{
		auto& src = src_planes[i];
		auto& dst = planes[i];

		int bits = 0;
		for (int j = 0; j < 3; ++j)
		{
			dst.normal[j] = src.normal[j];
			if (src.normal[j] < 0) {
				bits |= 1 << j;
			}
		}

		dst.dist = src.dist;
		dst.type = src.type;
		dst.signbits = bits;
	}
}

void BspLoader::LoadTexInfo(std::ifstream& fin, const BspFileLump& lump,
	                        std::vector<BspModel::TexInfo>& info,
	                        const std::vector<BspModel::Texture>& textures)
{
	int n = lump.size / sizeof(BspTexInfo);

	std::vector<BspTexInfo> src_info;
	src_info.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_info[0]), lump.size);

	info.resize(n);
	for (int i = 0; i < n; ++i)
	{
		auto& src = src_info[i];
		auto& dst = info[i];
		for (int j = 0; j < 4; ++j)
		{
			dst.vecs[0][j] = src.vecs[0][j];
			dst.vecs[1][j] = src.vecs[1][j];
		}
		float len1 = sm::vec3(dst.vecs[0]).Length();
		float len2 = sm::vec3(dst.vecs[1]).Length();
		len1 = (len1 + len2)/2;
		if (len1 < 0.32f)
			dst.mipadjust = 4;
		else if (len1 < 0.49f)
			dst.mipadjust = 3;
		else if (len1 < 0.99f)
			dst.mipadjust = 2;
		else
			dst.mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001f)
			dst.mipadjust = 1;		// don't crash
		else
			dst.mipadjust = 1 / floor((len1+len2)/2 + 0.1);
#endif
		dst.flags = src.flags;
		//assert(src.miptex >= 0
		//	&& src.miptex < static_cast<int>(textures.size())
		//	&& textures[src.miptex] != nullptr);
		dst.tex_idx = src.miptex;
	}
}

void BspLoader::LoadFaces(std::ifstream& fin, const BspFileLump& lump,
	                      BspModel& model)
{
	int n = lump.size / sizeof(BspFace);

	std::vector<BspFace> src_faces;
	src_faces.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_faces[0]), lump.size);

	model.surfaces.resize(n);
	for (int i = 0; i < n; ++i)
	{
		auto& src = src_faces[i];
		auto& dst = model.surfaces[i];

		dst.firstedge = src.firstedge;
		dst.numedges  = src.numedges;
		for (int j = 0; j < MAXLIGHTMAPS; ++j) {
			dst.styles[j] = src.styles[j];
		}

		dst.flags = 0;
		if (src.side) {
			dst.flags |= SURF_PLANEBACK;
		}

		dst.plane_idx = src.planenum;

		dst.tex_info_idx = src.texinfo;

		CalcSurfaceExtents(dst, model);
		CalcSurfaceBounds(dst, model);

		if (src.lightofs == -1) {
			dst.samples = nullptr;
		} else {
			dst.samples = model.lightdata + (src.lightofs * 3);
		}

		dst.polys = nullptr;
		dst.next = nullptr;

		// todo set flags
	}
}

void BspLoader::LoadMarkSurfaces(std::ifstream& fin, const BspFileLump& lump, BspModel& model)
{
	int n = lump.size / sizeof(uint16_t);

	std::vector<uint16_t> src_indices;
	src_indices.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_indices[0]), lump.size);

	model.mark_surfaces = reinterpret_cast<BspModel::Surface**>(new char[sizeof(BspModel::Surface**) * n]);
	for (int i = 0; i < n; ++i) {
		model.mark_surfaces[i] = &model.surfaces[src_indices[i]];
	}
}

uint8_t* BspLoader::LoadVisibility(std::ifstream& fin, const BspFileLump& lump)
{
	if (lump.size == 0) {
		return nullptr;
	}

	uint8_t* data = new uint8_t[lump.size];
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(data), lump.size);
	return data;
}

void BspLoader::LoadLeafs(std::ifstream& fin, const BspFileLump& lump, BspModel& model)
{
	int n = lump.size / sizeof(BspLeaf);

	std::vector<BspLeaf> src_leafs;
	src_leafs.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_leafs[0]), lump.size);

	model.leafs.resize(n);
	for (int i = 0; i < n; ++i)
	{
		auto& src = src_leafs[i];
		auto& dst = model.leafs[i];

		for (int j = 0 ; j < 3 ; ++j)
		{
			dst.minmaxs[j]   = src.mins[j];
			dst.minmaxs[3+j] = src.maxs[j];
		}

		dst.contents = src.contents;

		dst.firstmarksurface = model.mark_surfaces + (unsigned short)src.firstmarksurface; //johnfitz -- unsigned short
		dst.nummarksurfaces = (unsigned short)src.nummarksurfaces; //johnfitz -- unsigned short

		if (src.visofs == -1) {
			dst.compressed_vis = NULL;
		} else {
			dst.compressed_vis = model.visdata + src.visofs;
		}
//		dst.efrags = NULL;

		for (int j = 0; j < 4; ++j) {
			dst.ambient_sound_level[j] = src.ambient_level[j];
		}
	}
}

void BspLoader::LoadNodes(std::ifstream& fin, const BspFileLump& lump, BspModel& model)
{
	int n = lump.size / sizeof(BspNode);

	std::vector<BspNode> src_nodes;
	src_nodes.resize(n);
	fin.seekg(lump.offset);
	fin.read(reinterpret_cast<char*>(&src_nodes[0]), lump.size);

	model.nodes.resize(n);
	for (int i = 0; i < n; ++i)
	{
		auto& src = src_nodes[i];
		auto& dst = model.nodes[i];

		for (int j = 0; j < 3; ++j)
		{
			dst.minmaxs[j]   = src.mins[j];
			dst.minmaxs[3+j] = src.maxs[j];
		}

		dst.plane = &model.planes[src.planenum];

		dst.firstsurface = (unsigned short)src.firstface; //johnfitz -- explicit cast as unsigned short
		dst.numsurfaces = (unsigned short)src.numfaces; //johnfitz -- explicit cast as unsigned short

		for (int j = 0; j < 2; ++j)
		{
			//johnfitz -- hack to handle nodes > 32k, adapted from darkplaces
			uint16_t p = static_cast<uint16_t>(src.children[j]);
			if (p < n) {
				dst.children[j] = &model.nodes[p];
			} else {
				p = 65535 - p; //note this uses 65535 intentionally, -1 is leaf 0
				if (p < static_cast<int>(model.leafs.size())) {
					dst.children[j] = (BspModel::Node*)(&model.leafs[p]);
				} else {
					dst.children[j] = (BspModel::Node*)(&model.leafs[0]); //map it to the solid leaf
				}
			}
			//johnfitz
		}
	}
}

void BspLoader::LoadClipnodes(std::ifstream& fin, const BspFileLump& lump, BspModel& model)
{
	std::vector<BspClipnode> src_nodes;

	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspEdge);
	src_nodes.resize(n);
	fin.read(reinterpret_cast<char*>(&src_nodes[0]), lump.size);

	model.clip_nodes.resize(n);

	{
		auto& hull = model.hulls[1];
		hull.clipnodes = &model.clip_nodes[0];
		hull.firstclipnode = 0;
		hull.lastclipnode = n - 1;
		hull.planes = &model.planes[0];
		hull.clip_mins[0] = -16;
		hull.clip_mins[1] = -16;
		hull.clip_mins[2] = -24;
		hull.clip_maxs[0] = 16;
		hull.clip_maxs[1] = 16;
		hull.clip_maxs[2] = 32;
	}
	{
		auto& hull = model.hulls[2];
		hull.clipnodes = &model.clip_nodes[0];
		hull.firstclipnode = 0;
		hull.lastclipnode = n - 1;
		hull.planes = &model.planes[0];
		hull.clip_mins[0] = -32;
		hull.clip_mins[1] = -32;
		hull.clip_mins[2] = -24;
		hull.clip_maxs[0] = 32;
		hull.clip_maxs[1] = 32;
		hull.clip_maxs[2] = 64;
	}

	for (int i = 0; i < n; ++i)
	{
		auto& src = src_nodes[i];
		auto& dst = model.clip_nodes[i];

		dst.planenum = src.planenum;

		//johnfitz -- bounds check
		if (dst.planenum < 0 || dst.planenum >= static_cast<int>(model.planes.size()))
		{
			//johnfitz

				//johnfitz -- support clipnodes > 32k
			dst.children[0] = (unsigned short)src.children[0];
			dst.children[1] = (unsigned short)src.children[1];

			if (dst.children[0] >= n)
				dst.children[0] -= 65536;
			if (dst.children[1] >= n)
				dst.children[1] -= 65536;
			//johnfitz
		}
	}
}

// todo
int8_t* BspLoader::LoadEntities(std::ifstream& fin, const BspFileLump& lump)
{
	return nullptr;
}

void BspLoader::LoadSubmodels(std::ifstream& fin, const BspFileLump& lump,
	                          std::vector<BspSubmodel>& submodels)
{
	fin.seekg(lump.offset);
	int n = lump.size / sizeof(BspSubmodel);
	submodels.resize(n);
	fin.read(reinterpret_cast<char*>(&submodels[0]), lump.size);
}

void BspLoader::CalcSurfaceExtents(BspModel::Surface& s, const BspModel& model)
{
	float mins[2], maxs[2];
	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	auto& tex = model.tex_info[s.tex_info_idx];
	for (int i = 0; i < s.numedges; ++i)
	{
		int e = model.surface_edges[s.firstedge + i];
		const BspVertex* v = nullptr;
		if (e >= 0) {
			v = &model.vertices[model.edges[e].v[0]];
		} else {
			v = &model.vertices[model.edges[-e].v[1]];
		}

		for (int j = 0; j < 2; ++j)
		{
			/* The following calculation is sensitive to floating-point
			 * precision.  It needs to produce the same result that the
			 * light compiler does, because R_BuildLightMap uses surf->
			 * extents to know the width/height of a surface's lightmap,
			 * and incorrect rounding here manifests itself as patches
			 * of "corrupted" looking lightmaps.
			 * Most light compilers are win32 executables, so they use
			 * x87 floating point.  This means the multiplies and adds
			 * are done at 80-bit precision, and the result is rounded
			 * down to 32-bits and stored in val.
			 * Adding the casts to double seems to be good enough to fix
			 * lighting glitches when Quakespasm is compiled as x86_64
			 * and using SSE2 floating-point.  A potential trouble spot
			 * is the hallway at the beginning of mfxsp17.  -- ericw
			 */
			float val =	(v->point[0] * tex.vecs[j][0]) +
				        (v->point[1] * tex.vecs[j][1]) +
				        (v->point[2] * tex.vecs[j][2]) +
				        tex.vecs[j][3];

			if (val < mins[j]) {
				mins[j] = val;
			}
			if (val > maxs[j]) {
				maxs[j] = val;
			}
		}
	}

	int	bmins[2], bmaxs[2];
	for (int i = 0; i < 2; ++i)
	{
		bmins[i] = static_cast<int>(floor(mins[i] / 16));
		bmaxs[i] = static_cast<int>(ceil(maxs[i] / 16));

		s.texturemins[i] = bmins[i] * 16;
		s.extents[i] = (bmaxs[i] - bmins[i]) * 16;
	}
}

void BspLoader::CalcSurfaceBounds(BspModel::Surface& s, const BspModel& model)
{
	s.mins[0] = s.mins[1] = s.mins[2] = 9999;
	s.maxs[0] = s.maxs[1] = s.maxs[2] = -9999;

	for (int i = 0; i < s.numedges; ++i)
	{
		int e = model.surface_edges[s.firstedge + i];
		const BspVertex* v = nullptr;
		if (e >= 0) {
			v = &model.vertices[model.edges[e].v[0]];
		} else {
			v = &model.vertices[model.edges[-e].v[1]];
		}

		if (s.mins[0] > v->point[0])
			s.mins[0] = v->point[0];
		if (s.mins[1] > v->point[1])
			s.mins[1] = v->point[1];
		if (s.mins[2] > v->point[2])
			s.mins[2] = v->point[2];

		if (s.maxs[0] < v->point[0])
			s.maxs[0] = v->point[0];
		if (s.maxs[1] < v->point[1])
			s.maxs[1] = v->point[1];
		if (s.maxs[2] < v->point[2])
			s.maxs[2] = v->point[2];
	}
}

void BspLoader::ChainSurfaceByTexture(BspModel& model)
{
	for (auto& node : model.nodes)
	{
		for (int i = 0; i < static_cast<int>(node.numsurfaces); ++i)
		{
			auto& s = model.surfaces[node.firstsurface + i];
			auto& tex = model.textures[model.tex_info[s.tex_info_idx].tex_idx];
			s.next = tex.surfaces_chain;
			tex.surfaces_chain = &s;
		}
	}
}

uint32_t BspLoader::BuildModelVertexBuffer(BspModel& model)
{
	int numverts = 0;
	for (auto& s : model.surfaces) {
		numverts += s.numedges;
	}

	int buf_sz = sizeof(float) * VERTEXSIZE * numverts;
	float* buf = reinterpret_cast<float*>(new uint8_t[buf_sz]);
	int idx = 0;
	for (auto& s : model.surfaces)
	{
		s.vbo_firstvert = idx;
		memcpy(&buf[VERTEXSIZE * idx], s.polys->verts, sizeof(float) * VERTEXSIZE * s.numedges);
		idx += s.numedges;
	}

	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
	auto vbo = rc.CreateBuffer(ur::VERTEXBUFFER, buf, buf_sz);
	delete[] buf;

	return vbo;
}

uint32_t BspLoader::BuildModelIndexBuffer(BspModel& model)
{
	int num = 0;
	for (auto& s : model.surfaces) {
		num += 3 * (s.numedges - 2);
	}

	int buf_sz = sizeof(uint16_t) * num;
	uint16_t* buf = new uint16_t[num];
	uint16_t* dst = buf;
	for (auto& s : model.surfaces)
	{
		for (int i = 2; i < s.numedges; ++i)
		{
			*dst++ = s.vbo_firstvert;
			*dst++ = s.vbo_firstvert + i - 1;
			*dst++ = s.vbo_firstvert + i;
		}
	}

	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
	auto ebo = rc.CreateBuffer(ur::INDEXBUFFER, buf, buf_sz);
	delete[] buf;

	return ebo;
}

}