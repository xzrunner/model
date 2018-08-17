#include "model/AssimpExporter.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

namespace
{

// default pp steps
unsigned int ppsteps = aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
	aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
	aiProcess_ValidateDataStructure    | // perform a full validation of the loader's output
	aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
	aiProcess_RemoveRedundantMaterials | // remove redundant materials
	aiProcess_FindDegenerates          | // remove degenerated polygons from the import
	aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
	aiProcess_GenUVCoords              | // convert spherical, cylindrical, box and planar mapping to proper UVs
	aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
	aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
	aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
	aiProcess_OptimizeMeshes		   | // join small meshes, if possible;
	aiProcess_SplitByBoneCount         | // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
	0;

}

namespace model
{

void AssimpExporter::Export(const std::string& src_path, const std::string& dst_path)
{
	Assimp::Importer importer;
	//const aiScene* ai_scene = importer.ReadFile(src_path.c_str(),
	//	ppsteps | /* configurable pp steps */
	//	aiProcess_GenSmoothNormals | // generate smooth normal vectors if not existing
	//	aiProcess_SplitLargeMeshes | // split large, unrenderable meshes into submeshes
	//	aiProcess_Triangulate | // triangulate polygons with more than 3 edges
	//	aiProcess_ConvertToLeftHanded | // convert everything to D3D left handed space
	//	aiProcess_SortByPType                // make 'clean' meshes which consist of a single typ of primitives
	//);
	const aiScene* ai_scene = importer.ReadFile(src_path.c_str(), aiProcess_ValidateDataStructure);

	if (!ai_scene) {
		return;
	}

	Assimp::Exporter exporter;
	int n = exporter.GetExportFormatCount();
	for (int i = 0; i < n; ++i) {
		auto desc = exporter.GetExportFormatDescription(i);
		printf("%s, %s, %s\n", desc->description, desc->fileExtension, desc->id);
		int zz = 0;
	}

	int n_anim = ai_scene->mNumAnimations;

	//exporter.Export(ai_scene, "x", dst_path, aiProcess_FlipUVs);
	exporter.Export(ai_scene, "collada", dst_path);
	//exporter.Export(ai_scene, "3ds", dst_path, ppsteps | aiProcess_FlipUVs);

	/////

	int n_anim2;
	{
		Assimp::Importer importer;
		const aiScene* ai_scene = importer.ReadFile(dst_path.c_str(), aiProcess_ValidateDataStructure);

		if (!ai_scene) {
			return;
		}
		n_anim2 = ai_scene->mNumAnimations;
	}
}

}