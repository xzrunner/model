#include "model/Scene.h"
#include "model/SurfaceLoader.h"
#include "model/ObjLoader.h"
#include "model/M3DLoader.h"
#include "model/AssimpHelper.h"

#include <guard/check.h>

#include <boost/filesystem.hpp>

namespace model
{

bool Scene::LoadFromFile(const std::string& filepath)
{
	auto ext = boost::filesystem::extension(filepath);
	if (ext == ".param") {
		return SurfaceLoader::Load(*this, filepath);
	} else if (ext == ".obj") {
//		return ObjLoader::Load(*this, filepath);
		return AssimpHelper::Load(*this, filepath);
	} else if (ext == ".m3d") {
		return M3DLoader::Load(*this, filepath);
	} else if (ext == ".X") {
		return AssimpHelper::Load(*this, filepath);
	} else {
		GD_REPORT_ASSERT("unknown type.");
		return false;
	}

	return false;
}

}