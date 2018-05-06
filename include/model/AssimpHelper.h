#pragma once

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

class Model;
struct Mesh;
struct Material;

class AssimpHelper
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	static void LoadNode(const aiScene* scene, const aiNode* node, Model& model,
		const std::string& dir, pt3::AABB& aabb);

	static std::unique_ptr<Mesh> LoadMesh(const aiMesh* ai_mesh, const aiMaterial* ai_material,
		const std::string& dir, const sm::mat4& trans, pt3::AABB& aabb);

	static void LoadMaterial(const aiMesh* ai_mesh, const aiMaterial* ai_material,
		Mesh& mesh, const std::string& dir);

}; // AssimpHelper

}