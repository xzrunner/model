#include "model/MeshGeometry.h"

namespace model
{

MeshGeometry::~MeshGeometry()
{
    if (vert_buf) {
        delete[] vert_buf;
    }
}

//////////////////////////////////////////////////////////////////////////
// struct MeshRawData
//////////////////////////////////////////////////////////////////////////

void MeshRawData::CalcNormals()
{
	normals.resize(vertices.size());
	for (auto& f : faces)
	{
		auto n = (vertices[f[1]] - vertices[f[0]]).Cross(vertices[f[2]] - vertices[f[0]]);
		for (auto& v : f) {
			normals[v] += n;
		}
	}
	for (auto& n : normals) {
		n.Normalize();
	}
}


//////////////////////////////////////////////////////////////////////////
// struct BlendShapeData
//////////////////////////////////////////////////////////////////////////

#ifdef BLENDSHAPE_COMPRESS_FLOAT
void BlendShapeData::SetVertices(const std::vector<sm::vec3>& ori_verts,
                                 const std::vector<sm::vec3>& new_verts)
{
    assert(ori_verts.size() == new_verts.size());
    std::vector<sm::vec3> off_verts;
    off_verts.resize(ori_verts.size());
    for (size_t i = 0, n = ori_verts.size(); i < n; ++i) {
        off_verts[i] = new_verts[i] - ori_verts[i];
    }

    // calc flt region
    flt_min = FLT_MAX;
    flt_max = -FLT_MAX;
    for (auto& v : off_verts) {
        for (int i = 0; i < 3; ++i) {
            auto& f = v.xyz[i];
            if (f < flt_min) {
                flt_min = f;
            }
            if (f > flt_max) {
                flt_max = f;
            }
        }
    }

#ifdef BLENDSHAPE_COMPRESS_TO8
    const float dt = (flt_max - flt_min) / 0xff;
#else
    const float dt = (flt_max - flt_min) / 0xffff;
#endif // BLENDSHAPE_COMPRESS_TO8

    int ptr = -1;
    for (int i = 0, n = off_verts.size(); i < n; ++i)
    {
        if (off_verts[i] == sm::vec3(0, 0, 0))
        {
            if (ptr >= 0) {
                for (int j = ptr; j < i; ++j) {
                    for (int k = 0; k < 3; ++k) {
#ifdef BLENDSHAPE_COMPRESS_TO8
                        off_verts_idx.push_back(static_cast<uint8_t>((off_verts[j].xyz[k] - flt_min) / dt + 0.5f));
#else
                        off_verts_idx.push_back(static_cast<uint16_t>((off_verts[j].xyz[k] - flt_min) / dt + 0.5f));
#endif // BLENDSHAPE_COMPRESS_TO8
                    }
                }
                idx_verts.push_back((ptr << 16) | (i - ptr));

                ptr = -1;
            }
        }
        else
        {
            if (ptr < 0) {
                ptr = i;
            }
        }
    }
    if (ptr >= 0) {
        for (int i = ptr, n = off_verts.size(); i < n; ++i) {
            for (int j = 0; j < 3; ++j) {
#ifdef BLENDSHAPE_COMPRESS_TO8
                off_verts_idx.push_back(static_cast<uint8_t>((off_verts[i].xyz[j] - flt_min) / dt + 0.5f));
#else
                off_verts_idx.push_back(static_cast<uint16_t>((off_verts[i].xyz[j] - flt_min) / dt + 0.5f));
#endif // BLENDSHAPE_COMPRESS_TO8
            }
        }
        idx_verts.push_back((ptr << 16) | (ori_verts.size() - ptr));
    }

    //// stat
    //static int n_ori_vert = 0;
    //n_ori_vert += ori_verts.size();

    //static int n_c_off = 0;
    //n_c_off += off_verts_idx.size();

    //static int n_c_idx = 0;
    //n_c_idx += idx_verts.size();

    //printf("vert %d, coff %d, cidx %d\n", n_ori_vert, n_c_off, n_c_idx);
}
#else
void BlendShapeData::SetVertices(const std::vector<sm::vec3>& ori_verts,
                                 const std::vector<sm::vec3>& new_verts)
{
    assert(ori_verts.size() == new_verts.size());
    int ptr = -1;
    for (int i = 0, n = ori_verts.size(); i < n; ++i)
    {
        if (ori_verts[i] == new_verts[i])
        {
            if (ptr >= 0) {
                for (int j = ptr; j < i; ++j) {
                    off_verts.push_back(new_verts[j] - ori_verts[j]);
                }
                idx_verts.push_back((ptr << 16) | (i - ptr));

                ptr = -1;
            }
        }
        else
        {
            if (ptr < 0) {
                ptr = i;
            }
        }
    }

    if (ptr >= 0) {
        for (int i = ptr, n = ori_verts.size(); i < n; ++i) {
            off_verts.push_back(new_verts[i] - ori_verts[i]);
        }
        idx_verts.push_back((ptr << 16) | (ori_verts.size() - ptr));
    }
}
#endif // BLENDSHAPE_COMPRESS_FLOAT

}