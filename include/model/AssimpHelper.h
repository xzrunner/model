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

namespace pt3 { class AABB; }

namespace model
{

struct Mesh;
struct Material;

class AssimpHelper
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	static int LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Model& model, SkeletalAnim& anim,
		const std::vector<pt3::AABB>& meshes_aabb, const sm::mat4& mat);

	static std::unique_ptr<Model::Mesh> LoadMesh(const aiMesh* ai_mesh, pt3::AABB& aabb);

	static std::unique_ptr<Model::Material> LoadMaterial(const aiMaterial* ai_material,
		Model& model, const std::string& dir);

	static int LoadTexture(Model& model, const std::string& filepath);

	static std::unique_ptr<SkeletalAnim::Animation> LoadAnimation(const aiAnimation* ai_anim);
	static std::unique_ptr<SkeletalAnim::NodeAnim> LoadNodeAnim(const aiNodeAnim* ai_node);

}; // AssimpHelper

}