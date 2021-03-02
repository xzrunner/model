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

#include <filesystem>

namespace model
{

bool Model::LoadFromFile(const std::string& filepath)
{
	auto ext = std::filesystem::path(filepath).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
	if (ext == ".param") {
		return SurfaceLoader::Load(*dev, *this, filepath);
	} else if (ext == ".obj") {
//		return ObjLoader::Load(*this, filepath);
		return AssimpHelper::Load(*dev, *this, filepath);
	} else if (ext == ".m3d") {
		return M3dLoader::Load(*dev, *this, filepath);
	} else if (ext == ".xml") {
		return MaxLoader::Load(*dev, *this, filepath);
	}
#ifndef NO_QUAKE
	else if (ext == ".mdl") {
		return MdlLoader::Load(*dev, *this, filepath);
	} else if (ext == ".bsp") {
		return BspLoader::Load(*dev, *this, filepath);
    } else if (ext == ".map") {
        return MapBuilder::Load(*dev, *this, filepath);
    //} else if (ext == ".fbx") {
    //    return FbxLoader::Load(*this, filepath);
	} 
#endif // NO_QUAKE
	else {
//		return AssimpHelper::Load(*this, filepath, 1, true, 0xffffffff);

        float scale = 1.0f;
        if (ext == ".fbx") {
            scale = 0.01f;
        }
		bool ret = AssimpHelper::Load(*dev, *this, filepath, scale);

        // load blendshape
        if (ext == ".fbx")
        {
            std::vector<std::unique_ptr<BlendShapeLoader::MeshData>> meshes;
            FbxLoader::LoadBlendShapeMeshes(meshes, filepath);
            BlendShapeLoader::Load(*this, meshes);
        }

        return ret;
	}

	return false;
}

}