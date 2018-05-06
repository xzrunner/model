#include "model/MeshGeometry.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

namespace model
{

MeshGeometry::~MeshGeometry()
{
	ur::Blackboard::Instance()->GetRenderContext().ReleaseVAO(vao, vbo, ebo);
}

}