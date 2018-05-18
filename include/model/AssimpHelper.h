#pragma once

#include "Scene.h"

#include <SM_Matrix.h>

#include <map>
#include <memory>

struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;

namespace pt3 { class AABB; }

namespace model
{

struct Mesh;
struct Material;

class AssimpHelper
{
public:
	static bool Load(Scene& scene, const std::string& filepath);

private:
	static int LoadNode(const aiScene* ai_scene, const aiNode* node, Scene& scene,
		const std::vector<pt3::AABB>& meshes_aabb, const sm::mat4& mat);

	static std::unique_ptr<Scene::Mesh> LoadMesh(const aiMesh* ai_mesh, pt3::AABB& aabb);

	static std::unique_ptr<Scene::Material> LoadMaterial(const aiMaterial* ai_material,
		Scene& scene, const std::string& dir);

	static int LoadTexture(Scene& scene, const std::string& filepath);

}; // AssimpHelper

}