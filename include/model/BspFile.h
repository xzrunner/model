#pragma once

#include <cstdint>

namespace model
{

#define	MAX_MAP_HULLS		4

#define BSPVERSION	29

/* RMQ support (2PSB). 32bits instead of shorts for all but bbox sizes (which
 * still use shorts) */
#define BSP2VERSION_2PSB (('B' << 24) | ('S' << 16) | ('P' << 8) | '2')

/* BSP2 support. 32bits instead of shorts for everything (bboxes use floats) */
#define BSP2VERSION_BSP2 (('B' << 0) | ('S' << 8) | ('P' << 16) | ('2'<<24))

#define	LUMP_ENTITIES	0
#define	LUMP_PLANES		1
#define	LUMP_TEXTURES	2
#define	LUMP_VERTEXES	3
#define	LUMP_VISIBILITY	4
#define	LUMP_NODES		5
#define	LUMP_TEXINFO	6
#define	LUMP_FACES		7
#define	LUMP_LIGHTING	8
#define	LUMP_CLIPNODES	9
#define	LUMP_LEAFS		10
#define	LUMP_MARKSURFACES 11
#define	LUMP_EDGES		12
#define	LUMP_SURFEDGES	13
#define	LUMP_MODELS		14

#define	HEADER_LUMPS	15

struct BspSubmodel
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLS];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
};

struct BspFileLump
{
	int offset;
	int size;
};

struct BspHeader
{
	int      version;
	BspFileLump lumps[HEADER_LUMPS];
};

struct BspMipTexLump
{
	int nummiptex;
	int dataofs[1];
};

#define	MIPLEVELS	4
struct BspMipTex
{
	char     name[16];
	unsigned width, height;
	unsigned offsets[MIPLEVELS];		// four mip maps stored
};

struct BspVertex
{
	float point[3];
};

struct BspEdge
{
//	unsigned int	v[2];		// vertex numbers
	unsigned short	v[2];		// vertex numbers
};

// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

struct BspPlane
{
	float normal[3];
	float dist;
	int   type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
};

struct BspTexInfo
{
	float vecs[2][4];		// [s/t][xyz offset]
	int   miptex;
	int	  flags;
};

#define	MAXLIGHTMAPS	4
struct BspFace
{
	int16_t		planenum;
	int16_t		side;

	int			firstedge;		// we must support > 64k edges
	int16_t		numedges;
	int16_t		texinfo;

	// lighting info
	uint8_t		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
};

struct BspNode
{
	int			planenum;
	int16_t		children[2];	// negative numbers are -(leafs+1), not nodes
	int16_t		mins[3];		// for sphere culling
	int16_t		maxs[3];
	uint16_t	firstface;
	uint16_t	numfaces;	// counting both sides
};

#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
struct BspLeaf
{
	int		 contents;
	int		 visofs;				// -1 = no visibility info

	int16_t	 mins[3];			// for frustum culling
	int16_t	 maxs[3];

	uint16_t firstmarksurface;
	uint16_t nummarksurfaces;

	uint8_t	 ambient_level[NUM_AMBIENTS];
};

struct BspClipnode
{
	int	  planenum;
	short children[2];	// negative numbers are contents
};

}