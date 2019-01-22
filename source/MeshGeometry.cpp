#include "model/MeshGeometry.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

namespace model
{

MeshGeometry::~MeshGeometry()
{
	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
	if (vao != 0) {
		rc.ReleaseVAO(vao, vbo, ebo);
	} else {
        if (vbo != 0) {
            rc.ReleaseBuffer(ur::VERTEXBUFFER, vbo);
        }
		if (ebo != 0) {
			rc.ReleaseBuffer(ur::INDEXBUFFER, ebo);
		}
	}

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

    //// stat
    //float min = FLT_MAX, max = -FLT_MAX;
    //for (auto& v : off_verts) {
    //    for (int i = 0; i < 3; ++i) {
    //        auto& f = v.xyz[i];
    //        if (f < min) {
    //            min = f;
    //        }
    //        if (f > max) {
    //            max = f;
    //        }
    //    }
    //}
    //int zz = 0;
}

}