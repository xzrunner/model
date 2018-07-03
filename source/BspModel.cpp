#include "model/BspModel.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

namespace
{

const float SCALE = 0.01f;

}

namespace model
{

void BspModel::BuildSurfaceDisplayList()
{
	int idx = 0;
	for (auto& s : surfaces)
	{
		auto poly = reinterpret_cast<Poly*>(
			new char[sizeof(Poly) + (s.numedges - 1) * VERTEXSIZE * sizeof(float)]);

		poly->next = nullptr;
		poly->chain = nullptr;

		poly->numverts = s.numedges;
		for (int i = 0; i < s.numedges; ++i)
		{
			BspEdge* redge = nullptr;
			BspVertex* vec = nullptr;

			int lindex = surface_edges[s.firstedge + i];
			if (lindex > 0) {
				redge = &edges[lindex];
				vec = &vertices[redge->v[0]];
			} else {
				redge = &edges[-lindex];
				vec = &vertices[redge->v[1]];
			}

			poly->verts[i][0] = vec->point[0] * SCALE;
			poly->verts[i][1] = vec->point[1] * SCALE;
			poly->verts[i][2] = vec->point[2] * SCALE;

			auto& ti = tex_info[s.tex_info_idx];
			auto& tex = textures[ti.tex_idx].tex;
			if (tex)
			{
				float s = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[0])) + ti.vecs[0][3];
				s /= tex->Width();
				float t = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[1])) + ti.vecs[1][3];
				t /= tex->Height();
				poly->verts[i][3] = s;
				poly->verts[i][4] = t;
			}
			else
			{
				poly->verts[i][3] = 0;
				poly->verts[i][4] = 0;
			}

			// todo lightmap texture coordinates
			poly->verts[i][5] = 0;
			poly->verts[i][6] = 0;
		}

		s.polys = poly;

		++idx;
	}
}

}