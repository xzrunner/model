#include "model/Mesh.h"
#include "model/Callback.h"

namespace model
{

Mesh::~Mesh()
{
	for (auto& mat : materials) {
		Callback::ReleaseImg(mat.texture);
	}
	for (auto& mat : old_materials) {
		Callback::ReleaseImg(mat.texture);
	}
}

}