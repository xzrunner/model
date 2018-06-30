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
}

}