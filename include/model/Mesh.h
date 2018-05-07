#pragma once

#include "model/MeshGeometry.h"
#include "model/Material.h"

#include <boost/noncopyable.hpp>

namespace model
{

struct Mesh : boost::noncopyable
{
	MeshGeometry geometry;

	std::vector<Material>    materials;
	std::vector<MaterialOld> old_materials;

}; // Mesh

}