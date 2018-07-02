#pragma once

namespace model
{

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

struct FileLump
{
	int offset;
	int size;
};

struct BspHeader
{
	int      version;
	FileLump lumps[HEADER_LUMPS];
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

}