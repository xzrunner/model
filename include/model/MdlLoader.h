#pragma once

#include <SM_Vector.h>

#include <string>

namespace model
{

struct Model;

class MdlLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	struct MdlHeader
	{
		char magic[4];        /* magic number: "IDPO" */
		int version;          /* version: 6 */

		sm::vec3 scale;       /* scale factor */
		sm::vec3 translate;   /* translation vector */
		float boundingradius;
		sm::vec3 eyeposition; /* eyes' position */

		int num_skins;        /* number of textures */
		int skinwidth;        /* texture width */
		int skinheight;       /* texture height */

		int num_verts;        /* number of vertices */
		int num_tris;         /* number of triangles */
		int num_frames;       /* number of frames */

		int synctype;         /* 0 = synchron, 1 = random */
		int flags;            /* state flag */
		float size;
	};

	struct MdlSkin
	{
		int group;      /* 0 = single, 1 = group */
		char* data;     /* texture data */
	};

	struct MdlTexcoord
	{
		int onseam;
		int s;
		int t;
	};

	struct MdlTriangle
	{
		int facesfront;  /* 0 = backface, 1 = frontface */
		int vertex[3];   /* vertex indices */
	};

	struct MdlVertex
	{
		unsigned char v[3];
		unsigned char normal_idx;
	};

	struct MdlSimpleframe
	{
		struct MdlVertex bboxmin; /* bouding box min */
		struct MdlVertex bboxmax; /* bouding box max */
		char name[16];
		struct MdlVertex *verts;  /* vertex list of the frame */
	};

	/* Model frame */
	struct MdlFrame
	{
		int type;                     /* 0 = simple, !0 = group */
		struct MdlSimpleframe frame;  /* this program can't read models
										 composed of group frames! */
	};

private:
	static void LoadMaterial(const MdlHeader& header, std::ifstream& fin, Model& model, const std::string& filepath);
	static void LoadMesh(const MdlHeader& header, std::ifstream& fin, Model& model);

	static sm::vec3 TransVertex(const MdlVertex& vertex, const sm::vec3& scale, const sm::vec3& translate);

}; // MdlLoader

}