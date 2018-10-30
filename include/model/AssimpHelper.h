#pragma once

#include "model/Model.h"
#include "model/SkeletalAnim.h"

#include <SM_Matrix.h>

#include <map>
#include <memory>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;
struct aiAnimation;
struct aiNodeAnim;

namespace model
{

struct Mesh;
struct Material;

class AssimpHelper
{
public:
	static bool Load(Model& model, const std::string& filepath, float scale = 1.0f);

private:
	static int LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Model& model, 
		std::vector<std::unique_ptr<SkeletalAnim::Node>>& nodes,
		const std::vector<sm::cube>& meshes_aabb, const sm::mat4& mat);

	static std::unique_ptr<Model::Mesh> LoadMesh(const aiMesh* ai_mesh, sm::cube& aabb, float scale);

	static std::unique_ptr<Model::Material> LoadMaterial(const aiMaterial* ai_material,
		Model& model, const std::string& dir);

	static int LoadTexture(Model& model, const std::string& filepath);

	static std::unique_ptr<SkeletalAnim::ModelExtend> LoadAnimation(const aiAnimation* ai_anim);
	static std::unique_ptr<SkeletalAnim::NodeAnim> LoadNodeAnim(const aiNodeAnim* ai_node);

	static void CombineAABB(sm::cube& dst, const sm::cube& src, const sm::mat4& mat);

}; // AssimpHelper

}