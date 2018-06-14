#include "model/Model.h"
#include "model/SurfaceLoader.h"
#include "model/ObjLoader.h"
#include "model/M3DLoader.h"
#include "model/MaxLoader.h"
#include "model/AssimpHelper.h"
#include "model/Callback.h"

#include <guard/check.h>

#include <boost/filesystem.hpp>

namespace model
{

Model::~Model()
{
	for (auto& tex : textures) {
		Callback::ReleaseImg(tex.second);
	}
}

bool Model::LoadFromFile(const std::string& filepath)
{
	auto ext = boost::filesystem::extension(filepath);
	std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
	if (ext == ".param") {
		return SurfaceLoader::Load(*this, filepath);
	} else if (ext == ".obj") {
//		return ObjLoader::Load(*this, filepath);
		return AssimpHelper::Load(*this, filepath);
	} else if (ext == ".m3d") {
		return M3DLoader::Load(*this, filepath);
	} else if (ext == ".x") {
		return AssimpHelper::Load(*this, filepath);
	} else if (ext == ".xml") {
		return MaxLoader::Load(*this, filepath);
	} else {
		GD_REPORT_ASSERT("unknown type.");
		return false;
	}

	return false;
}

int Model::QueryNodeByName(const std::string& name) const
{
	for (int i = 0, n = nodes.size(); i < n; ++i) {
		if (nodes[i]->name == name) {
			return i;
		}
	}
	return -1;
}

}