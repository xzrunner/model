#pragma once

#include "model/MeshGeometry.h"
#include "model/Material.h"

#include <boost/noncopyable.hpp>

namespace model
{

struct Mesh : boost::noncopyable
{
	MeshGeometry geometry;

	Material    material;
	MaterialOld material_old;

}; // Mesh

}