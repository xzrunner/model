#pragma once

#include "model/ModelExtend.h"

#include <halfedge/Polyhedron.h>

namespace model
{

class HalfEdgeMesh : public ModelExtend
{
public:
	virtual ModelExtendType Type() const override { return EXT_HALFEDGE_MESH; }

public:
	std::vector<he::PolyhedronPtr> meshes;

}; // HalfEdgeMesh

}