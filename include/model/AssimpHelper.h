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

namespace ur { class Device; }

namespace model
{

struct Mesh;
struct MeshRawData;
struct Material;

class AssimpHelper
{
public:
	static bool Load(const ur::Device& dev, Model& model, const std::string& filepath, float scale = 1.0f);
    static bool Load(std::vector<std::unique_ptr<MeshRawData>>& meshes, const std::string& filepath);

    // config
    static void SetLoadRawData(bool load_raw_data) { m_load_raw_data = load_raw_data; }

private:
	static int LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Model& model,
		std::vector<std::unique_ptr<SkeletalAnim::Node>>& nodes,
		const std::vector<sm::cube>& meshes_aabb, const sm::mat4& mat);

	static std::unique_ptr<Model::Mesh> LoadMesh(const ur::Device& dev,
        const std::vector<std::unique_ptr<Model::Material>>& materials, const aiMesh* ai_mesh, sm::cube& aabb);
	static std::unique_ptr<MeshRawData> LoadMeshRawData(const aiMesh* ai_mesh);

	static std::unique_ptr<Model::Material>
        LoadMaterial(const ur::Device& dev, const aiMaterial* ai_material, Model& model, const std::string& dir);

	static int LoadTexture(const ur::Device& dev, Model& model, const std::string& filepath);

	static std::unique_ptr<SkeletalAnim::ModelExtend> LoadAnimation(const aiAnimation* ai_anim);
	static std::unique_ptr<SkeletalAnim::NodeAnim> LoadNodeAnim(const aiNodeAnim* ai_node);

	static void CombineAABB(sm::cube& dst, const sm::cube& src, const sm::mat4& mat);

private:
    static bool m_load_raw_data;
    static uint32_t m_vert_color;

}; // AssimpHelper

}