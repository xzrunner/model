#include "model/BspModel.h"

#include <quake/Lightmaps.h>
#include <unirender/Texture.h>

namespace
{

const float SCALE = 0.01f;

unsigned blocklights[quake::Lightmaps::BLOCK_WIDTH * quake::Lightmaps::BLOCK_HEIGHT * 3]; //johnfitz -- was 18*18, added lit support (*3) and loosened surface extents maximum (BLOCK_WIDTH*BLOCK_HEIGHT)

}

namespace model
{

void BspModel::CreateSurfaceLightmap(const ur::Device& dev)
{
	for (auto& s : surfaces)
	{
		int smax = (s.extents[0] >> 4) + 1;
		int tmax = (s.extents[1] >> 4) + 1;
		s.lightmaptexturenum = quake::Lightmaps::Instance()->AllocBlock(smax, tmax, &s.light_s, &s.light_t);
		uint8_t* data = quake::Lightmaps::Instance()->Query(s.lightmaptexturenum, s.light_s, s.light_t);
		BuildLightMap(s, data, quake::Lightmaps::BLOCK_WIDTH * quake::Lightmaps::BPP);
	}

	quake::Lightmaps::Instance()->CreatetTextures(dev);
}

void BspModel::BuildSurfaceDisplayList()
{
	int idx = 0;
	for (auto& surface : surfaces)
	{
		auto poly = reinterpret_cast<Poly*>(
			new char[sizeof(Poly) + (surface.numedges - 1) * VERTEXSIZE * sizeof(float)]);

		poly->next = nullptr;
		poly->chain = nullptr;

		poly->numverts = surface.numedges;
		for (int i = 0; i < surface.numedges; ++i)
		{
			BspEdge* redge = nullptr;
			BspVertex* vec = nullptr;

			// position

			int lindex = surface_edges[surface.firstedge + i];
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

			// texture coordinates

			auto& ti = tex_info[surface.tex_info_idx];
			auto& tex = textures[ti.tex_idx].tex;
			if (tex)
			{
				float s = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[0])) + ti.vecs[0][3];
				s /= tex->GetWidth();
				float t = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[1])) + ti.vecs[1][3];
				t /= tex->GetHeight();
				poly->verts[i][3] = s;
				poly->verts[i][4] = t;
			}
			else
			{
				poly->verts[i][3] = 0;
				poly->verts[i][4] = 0;
			}

			// lightmap texture coordinates

			float s, t;
			s = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[0])) + ti.vecs[0][3];
			s -= surface.texturemins[0];
			s += surface.light_s * 16;
			s += 8;
			s /= quake::Lightmaps::BLOCK_WIDTH * 16; //fa->texinfo->texture->width;

			t = sm::vec3(vec->point).Dot(sm::vec3(ti.vecs[1])) + ti.vecs[1][3];
			t -= surface.texturemins[1];
			t += surface.light_t * 16;
			t += 8;
			t /= quake::Lightmaps::BLOCK_HEIGHT * 16; //fa->texinfo->texture->height;

			poly->verts[i][5] = s;
			poly->verts[i][6] = t;
		}

		surface.polys = poly;

		++idx;
	}
}

void BspModel::BuildLightMap(Surface& surf, uint8_t* dest, int stride)
{
	int smax = (surf.extents[0] >> 4) + 1;
	int tmax = (surf.extents[1] >> 4) + 1;
	int size = smax * tmax;
	uint8_t* lightmap = surf.samples;
	if (lightdata)
	{
		// clear to no light
		memset(&blocklights[0], 0, size * 3 * sizeof(unsigned int)); //johnfitz -- lit support via lordhavoc

		// add all the lightmaps
		if (lightmap)
		{
			for (int maps = 0; maps < MAXLIGHTMAPS && surf.styles[maps] != 255; maps++)
			{
				int scale = 264;	// normal light value
//				int scale = d_lightstylevalue[surf.styles[maps]];
//				surf.cached_light[maps] = scale;	// 8.8 fraction
				//johnfitz -- lit support via lordhavoc
				unsigned* bl = blocklights;
				for (int i = 0; i < size; i++)
				{
					*bl++ += *lightmap++ * scale;
					*bl++ += *lightmap++ * scale;
					*bl++ += *lightmap++ * scale;
				}
				//johnfitz
			}
		}

		// add all the dynamic lights
		//if (surf->dlightframe == r_framecount)
		//	R_AddDynamicLights (surf);
	}
	else
	{
		// set to full bright if no light data
		memset(&blocklights[0], 255, size * 3 * sizeof(unsigned int)); //johnfitz -- lit support via lordhavoc
	}

	// bound, invert, and shift
	// store:
	int	r, g, b;
	stride -= smax * 4;
	unsigned* bl = blocklights;
	for (int i = 0; i < tmax; i++, dest += stride)
	{
		for (int j = 0; j < smax; j++)
		{
			//if (gl_overbright.value)
			//{
			//	r = *bl++ >> 8;
			//	g = *bl++ >> 8;
			//	b = *bl++ >> 8;
			//}
			//else
			{
				r = *bl++ >> 7;
				g = *bl++ >> 7;
				b = *bl++ >> 7;
			}
			*dest++ = (r > 255) ? 255 : r;
			*dest++ = (g > 255) ? 255 : g;
			*dest++ = (b > 255) ? 255 : b;

			*dest++ = 255;
		}
	}
}

}