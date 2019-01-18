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
		assert(vbo);
		rc.ReleaseBuffer(ur::VERTEXBUFFER, vbo);
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

}