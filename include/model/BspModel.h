#pragma once

#include "model/ModelExtend.h"
#include "model/BspFile.h"

#include <SM_Vector.h>
#include <unirender/Texture.h>

#include <vector>

namespace model
{

#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_NOTEXTURE		0x100 //johnfitz
#define SURF_DRAWFENCE		0x200
#define SURF_DRAWLAVA		0x400
#define SURF_DRAWSLIME		0x800
#define SURF_DRAWTELE		0x1000
#define SURF_DRAWWATER		0x2000

struct BspModel : public ModelExtend
{
	struct Plane
	{
		sm::vec3      normal;
		float         dist;
		unsigned char type;			// for texture axis selection and fast side tests
		unsigned char signbits;		// signx + signy<<1 + signz<<1
		unsigned char pad[2];
	};

	struct TexInfo
	{
		float vecs[2][4];
		float mipadjust;
		int   tex_idx;
		int   flags;
	};

#define	VERTEXSIZE	7

	struct Poly
	{
		Poly* next;
		Poly* chain;
		int   numverts;
		float verts[1][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
	};

	struct Surface
	{
		float		mins[3];		// johnfitz -- for frustum culling
		float		maxs[3];		// johnfitz -- for frustum culling

		uint32_t    plane_idx;
		int			flags;

		int			firstedge;	// look up in model->surfedges[], negative numbers
		int			numedges;	// are backwards edges

		uint16_t	texturemins[2];
		uint16_t	extents[2];

		Poly*       polys;
		Surface*    next;		// for surface chain

		uint32_t    tex_info_idx;

		int		    vbo_firstvert;		// index of this surface's first vert in the VBO

		uint8_t		styles[MAXLIGHTMAPS];

		uint8_t		*samples;		// [numstyles*surfsize]
	};

	struct Texture
	{
		ur::TexturePtr tex;
		Surface*       surfaces_chain;
	};

	struct Node
	{
		// common with leaf
		int			 contents;		// 0, to differentiate from leafs
		int			 visframe;		// node needs to be traversed if current

		float		 minmaxs[6];		// for bounding box culling

		Node*        parent;

		// node specific
		Plane*       plane;
		Node*        children[2];

		unsigned int firstsurface;
		unsigned int numsurfaces;
	};

	struct Leaf
	{
		// common with node
		int			contents;		// wil be a negative contents number
		int			visframe;		// node needs to be traversed if current

		float		minmaxs[6];		// for bounding box culling

		Node*       parent;

		// leaf specific
		uint8_t		*compressed_vis;
//		efrag_t		*efrags;

		Surface**   firstmarksurface;
		int			nummarksurfaces;
		int			key;			// BSP sequence number for leaf's contents
		uint8_t		ambient_sound_level[NUM_AMBIENTS];
	};

	struct Clipnode
	{
		int			planenum;
		int			children[2]; // negative numbers are contents
	};

	struct Hull
	{
		Clipnode*   clipnodes; //johnfitz -- was dclipnode_t
		Plane*      planes;
		int			firstclipnode;
		int			lastclipnode;
		float		clip_mins[3];
		float		clip_maxs[3];
	};

	std::vector<BspVertex> vertices;

	std::vector<BspEdge> edges;

	std::vector<int> surface_edges;

	std::vector<Texture> textures;

	uint8_t* visdata = nullptr;
	uint8_t* lightdata = nullptr;
	int8_t*  entities = nullptr;

	std::vector<Plane> planes;

	std::vector<TexInfo> tex_info;

	std::vector<Surface> surfaces;

	Surface** mark_surfaces;

	std::vector<Leaf> leafs;
	std::vector<Node> nodes;

	std::vector<Clipnode> clip_nodes;

	Hull hulls[MAX_MAP_HULLS];

	std::vector<BspSubmodel> submodels;

	~BspModel()
	{
		if (visdata) {
			delete[] visdata;
		}
		if (lightdata) {
			delete[] lightdata;
		}
	}

	virtual ModelExtendType Type() const override { return EXT_BSP; }

	void BuildSurfaceDisplayList();

}; // BspModel

}