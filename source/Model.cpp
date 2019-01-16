#include "model/Model.h"
#include "model/SurfaceLoader.h"
#include "model/ObjLoader.h"
#include "model/M3dLoader.h"
#include "model/MaxLoader.h"
#include "model/AssimpHelper.h"
#include "model/MdlLoader.h"
#include "model/BspLoader.h"
#include "model/MapBuilder.h"
#include "model/FbxLoader.h"

#include <guard/check.h>

#include <boost/filesystem.hpp>

namespace model
{

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
		return M3dLoader::Load(*this, filepath);
	} else if (ext == ".xml") {
		return MaxLoader::Load(*this, filepath);
	} else if (ext == ".mdl") {
		return MdlLoader::Load(*this, filepath);
	} else if (ext == ".bsp") {
		return BspLoader::Load(*this, filepath);
    } else if (ext == ".map") {
        return MapBuilder::Load(*this, filepath);
    //} else if (ext == ".fbx") {
    //    return FbxLoader::Load(*this, filepath);
	} else {
//		return AssimpHelper::Load(*this, filepath, 1, true, 0xffffffff);
		bool ret = AssimpHelper::Load(*this, filepath);

        // load blendshape
        FbxLoader::LoadBlendShape(*this, filepath);

        return ret;
	}

	return false;
}

}