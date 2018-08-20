#include "model/AssimpHelper.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/EffectType.h"
#include "model/TextureLoader.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>
#include <unirender/VertexAttrib.h>
#include <SM_Matrix.h>
#include <SM_Cube.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <boost/filesystem.hpp>

namespace
{

sm::mat4 trans_ai_mat(const C_STRUCT aiMatrix4x4& ai_mat)
{
	sm::mat4 mat;
	mat.x[0]  = ai_mat.a1;
	mat.x[1]  = ai_mat.b1;
	mat.x[2]  = ai_mat.c1;
	mat.x[3]  = ai_mat.d1;
	mat.x[4]  = ai_mat.a2;
	mat.x[5]  = ai_mat.b2;
	mat.x[6]  = ai_mat.c2;
	mat.x[7]  = ai_mat.d2;
	mat.x[8]  = ai_mat.a3;
	mat.x[9]  = ai_mat.b3;
	mat.x[10] = ai_mat.c3;
	mat.x[11] = ai_mat.d3;
	mat.x[12] = ai_mat.a4;
	mat.x[13] = ai_mat.b4;
	mat.x[14] = ai_mat.c4;
	mat.x[15] = ai_mat.d4;
	return mat;
}

sm::vec3 trans_ai_vector3d(const C_STRUCT aiVector3D& ai_vec3)
{
	sm::vec3 vec3;
	vec3.x = ai_vec3.x;
	vec3.y = ai_vec3.y;
	vec3.z = ai_vec3.z;
	return vec3;
}

sm::Quaternion trans_ai_quaternion(const C_STRUCT aiQuaternion& ai_quat)
{
	sm::Quaternion quat;
	quat.x = ai_quat.x;
	quat.y = ai_quat.y;
	quat.z = ai_quat.z;
	quat.w = ai_quat.w;
	return quat;
}

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

bool AssimpHelper::Load(Model& model, const std::string& filepath, float scale)
{
	Assimp::Importer importer;
	const aiScene* ai_scene = importer.ReadFile(filepath.c_str(),
		ppsteps | /* configurable pp steps */
		aiProcess_GenSmoothNormals		   | // generate smooth normal vectors if not existing
		aiProcess_SplitLargeMeshes         | // split large, unrenderable meshes into submeshes
		aiProcess_Triangulate			   | // triangulate polygons with more than 3 edges
		aiProcess_ConvertToLeftHanded	   | // convert everything to D3D left handed space
		aiProcess_SortByPType                // make 'clean' meshes which consist of a single typ of primitives
		);

	if (!ai_scene) {
		return NULL;
	}

	auto ext = std::make_unique<SkeletalAnim>();

	// material
	auto dir = boost::filesystem::path(filepath).parent_path().string();
	model.materials.reserve(ai_scene->mNumMaterials);
	for (size_t i = 0; i < ai_scene->mNumMaterials; ++i)
	{
		auto src = ai_scene->mMaterials[i];
		model.materials.push_back(LoadMaterial(src, model, dir));
	}

	// mesh
	std::vector<sm::cube> meshes_aabb;
	meshes_aabb.reserve(ai_scene->mNumMeshes);
	model.meshes.reserve(ai_scene->mNumMeshes);
	for (size_t i = 0; i < ai_scene->mNumMeshes; ++i)
	{
		auto src = ai_scene->mMeshes[i];
		sm::cube aabb;
		model.meshes.push_back(LoadMesh(src, aabb, scale));
		meshes_aabb.push_back(aabb);
	}

	// node
	LoadNode(ai_scene, ai_scene->mRootNode, model, *ext, meshes_aabb, sm::mat4());

	// todo: load lights and cameras

	// bone
	for (auto& mesh : model.meshes) {
		for (auto& bone : mesh->geometry.bones) {
			bone.node = ext->QueryNodeByName(bone.name);
		}
	}

	// animation
	for (size_t i = 0; i < ai_scene->mNumAnimations; ++i) {
		auto src = ai_scene->mAnimations[i];
		ext->AddAnim(LoadAnimation(src));
	}

	// todo
	if (boost::filesystem::extension(filepath) == ".X") {
		model.anim_speed = 100;
	}

	model.ext = std::move(ext);

	model.scale = scale;

	return true;
}

int AssimpHelper::LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Model& model, SkeletalAnim& ext,
	                       const std::vector<sm::cube>& meshes_aabb, const sm::mat4& mat)
{
	auto node = std::make_unique<SkeletalAnim::Node>();
	auto node_raw = node.get();

	node_raw->name = ai_node->mName.C_Str();

	int node_id = ext.GetNodeSize();
	ext.AddNode(node);

	node_raw->local_trans = trans_ai_mat(ai_node->mTransformation);

	auto child_mat = mat * node_raw->local_trans;

	if (ai_node->mNumChildren)
	{
		for (size_t i = 0; i < ai_node->mNumChildren; ++i)
		{
			int child = LoadNode(ai_scene, ai_node->mChildren[i], model, ext, meshes_aabb, child_mat);
			node_raw->children.push_back(child);

			auto node = ext.GetNode(child);
			assert(node->parent == -1);
			node->parent = node_id;
		}
	}
	else
	{
		if (ai_node->mNumMeshes)
		{
			//sm::mat4 trans_mat;
			//const aiNode* node = ai_node;
			//while (node) {
			//	sm::mat4 node_trans;
			//	node_trans.x[0] = ai_node->mTransformation.a1;
			//	node_trans.x[1] = ai_node->mTransformation.b1;
			//	node_trans.x[2] = ai_node->mTransformation.c1;
			//	node_trans.x[3] = ai_node->mTransformation.d1;
			//	node_trans.x[4] = ai_node->mTransformation.a2;
			//	node_trans.x[5] = ai_node->mTransformation.b2;
			//	node_trans.x[6] = ai_node->mTransformation.c2;
			//	node_trans.x[7] = ai_node->mTransformation.d2;
			//	node_trans.x[8] = ai_node->mTransformation.a3;
			//	node_trans.x[9] = ai_node->mTransformation.b3;
			//	node_trans.x[10] = ai_node->mTransformation.c3;
			//	node_trans.x[11] = ai_node->mTransformation.d3;
			//	node_trans.x[12] = ai_node->mTransformation.a4;
			//	node_trans.x[13] = ai_node->mTransformation.b4;
			//	node_trans.x[14] = ai_node->mTransformation.c4;
			//	node_trans.x[15] = ai_node->mTransformation.d4;

			//	trans_mat = node_trans * trans_mat;

			//	node = node_raw->mParent;
			//}



			//for (size_t i = 0; i < ai_node->mNumMeshes; ++i)
			//{
			//	const aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
			//	const aiMaterial* ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

			//	auto mesh = LoadMesh(ai_mesh, trans_mat, aabb);
			//	model.meshes.push_back(std::move(mesh));
			//}

			node_raw->meshes.reserve(ai_node->mNumMeshes);
			for (size_t i = 0; i < ai_node->mNumMeshes; ++i)
			{
				auto mesh = ai_node->mMeshes[i];
				node_raw->meshes.push_back(mesh);
				CombineAABB(model.aabb, meshes_aabb[mesh], child_mat);
			}
		}
	}

	return node_id;
}

std::unique_ptr<Model::Mesh> AssimpHelper::LoadMesh(const aiMesh* ai_mesh, sm::cube& aabb, float scale)
{
	auto mesh = std::make_unique<Model::Mesh>();

	mesh->material = ai_mesh->mMaterialIndex;

	int vertex_type = 0;
	int floats_per_vertex = 3;

	bool has_normal = ai_mesh->HasNormals();
	if (has_normal) {
		floats_per_vertex += 3;
		vertex_type |= VERTEX_FLAG_NORMALS;
	}

	bool has_texcoord = ai_mesh->HasTextureCoords(0);
	if (has_texcoord)
	{
		floats_per_vertex += 2;
		vertex_type |= VERTEX_FLAG_TEXCOORDS;
		mesh->effect = EFFECT_DEFAULT;
	}
	else
	{
		mesh->effect = EFFECT_DEFAULT_NO_TEX;
	}

	bool has_skinned = ai_mesh->HasBones();
	if (has_skinned)
	{
		floats_per_vertex += 2;
		vertex_type |= VERTEX_FLAG_SKINNED;
		mesh->effect = EFFECT_SKINNED;
	}

	std::vector<std::vector<std::pair<int, float>>> weights_per_vertex(ai_mesh->mNumVertices);
	for (unsigned int i = 0; i < ai_mesh->mNumBones; ++i)
	{
		auto bone = ai_mesh->mBones[i];
		for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
			weights_per_vertex[bone->mWeights[j].mVertexId].push_back({ i, bone->mWeights[j].mWeight });
		}
	}

	uint8_t* buf = new uint8_t[ai_mesh->mNumVertices * floats_per_vertex * sizeof(float)];
	uint8_t* ptr = buf;
	for (size_t i = 0; i < ai_mesh->mNumVertices; ++i)
	{
		const aiVector3D& p = ai_mesh->mVertices[i];

		sm::vec3 p_trans(p.x, p.y, p.z);
		p_trans *= scale;
		memcpy(ptr, &p_trans.x, sizeof(float) * 3);
		ptr += sizeof(float) * 3;
		aabb.Combine(p_trans);

		if (has_normal)
		{
			const aiVector3D& n = ai_mesh->mNormals[i];
			memcpy(ptr, &n.x, sizeof(float) * 3);
			ptr += sizeof(float) * 3;
		}
		if (has_texcoord)
		{
			const aiVector3D& t = ai_mesh->mTextureCoords[0][i];
			memcpy(ptr, &t.x, sizeof(float));
			ptr += sizeof(float);
			float y = 1 - t.y;
			memcpy(ptr, &y, sizeof(float));
			ptr += sizeof(float);
		}
		if (has_skinned)
		{
			assert(weights_per_vertex[i].size() <= 4);
			unsigned char indices[4] = { 0, 0, 0, 0 };
			unsigned char weights[4] = { 0, 0, 0, 0 };
			for (int j = 0, m = weights_per_vertex[i].size(); j < m; ++j)
			{
				indices[j] = weights_per_vertex[i][j].first;
				weights[j] = static_cast<unsigned char>(weights_per_vertex[i][j].second * 255.0f);
			}
			uint32_t indices_pack = (indices[3] << 24) | (indices[2] << 16) | (indices[1] << 8) | indices[0];
			uint32_t weights_pack = (weights[3] << 24) | (weights[2] << 16) | (weights[1] << 8) | weights[0];
			memcpy(ptr, &indices_pack, sizeof(float));
			ptr += sizeof(uint32_t);
			memcpy(ptr, &weights_pack, sizeof(float));
			ptr += sizeof(uint32_t);
		}
	}

	int count = 0;
	for (size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
		const aiFace& face = ai_mesh->mFaces[i];
		count += face.mNumIndices;
	}
	std::vector<uint16_t> indices;
	indices.reserve(count);

	for (size_t i = 0; i < ai_mesh->mNumFaces; ++i) {
		const aiFace& face = ai_mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	ur::RenderContext::VertexInfo vi;

	vi.vn = ai_mesh->mNumVertices;
	vi.vertices = buf;
	vi.stride = floats_per_vertex * sizeof(float);
	vi.in = indices.size();
	vi.indices = &indices[0];

	int stride = 0;
	// pos
	stride += 4 * 3;
	// normal
	if (has_normal) {
		stride += 4 * 3;
	}
	// texcoord
	if (has_texcoord) {
		stride += 4 * 2;
	}
	// skinned
	if (has_skinned) {
		stride += 4 + 4;
	}

	int offset = 0;
	// pos
	vi.va_list.push_back(ur::VertexAttrib("pos", 3, 4, stride, offset));
	offset += 4 * 3;
	// normal
	if (has_normal)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
		vi.va_list.push_back(ur::VertexAttrib("normal", 3, 4, stride, offset));
		offset += 4 * 3;
	}
	// texcoord
	if (has_texcoord)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
		vi.va_list.push_back(ur::VertexAttrib("texcoord", 2, 4, stride, offset));
		offset += 4 * 2;
	}
	// skinned
	if (has_skinned)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_SKINNED;
		vi.va_list.push_back(ur::VertexAttrib("blend_indices", 4, 1, stride, offset));
		offset += 4;
		vi.va_list.push_back(ur::VertexAttrib("blend_weights", 4, 1, stride, offset));
		offset += 4;
	}

	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, vi.in, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);

	mesh->geometry.bones.reserve(ai_mesh->mNumBones);
	for (size_t i = 0; i < ai_mesh->mNumBones; ++i)
	{
		auto src = ai_mesh->mBones[i];
		model::Bone dst;
		dst.node = -1;
		dst.name = src->mName.C_Str();
		dst.offset_trans = trans_ai_mat(src->mOffsetMatrix);

		mesh->geometry.bones.push_back(dst);
	}

	delete[] buf;

	return mesh;
}

std::unique_ptr<Model::Material> AssimpHelper::LoadMaterial(const aiMaterial* ai_material,
	                                                        Model& model, const std::string& dir)
{
	auto material = std::make_unique<Model::Material>();

	aiColor4D col;

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_DIFFUSE, &col) == AI_SUCCESS) {
		material->diffuse.x = col.r;
		material->diffuse.y = col.g;
		material->diffuse.z = col.b;
	} else {
		material->diffuse.x = material->diffuse.y = material->diffuse.z = 1;
	}

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_SPECULAR, &col) == AI_SUCCESS) {
		material->specular.x = col.r;
		material->specular.y = col.g;
		material->specular.z = col.b;
	} else {
		material->specular.x = material->specular.y = material->specular.z = 1;
	}

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_AMBIENT, &col) == AI_SUCCESS) {
		material->ambient.x = col.r;
		material->ambient.y = col.g;
		material->ambient.z = col.b;
	} else {
		material->ambient.x = material->ambient.y = material->ambient.z = 0.04f;
	}
	material->shininess = 50;

	material->diffuse_tex = -1;
//	if (ai_mesh->mTextureCoords[0])
	{
		aiString path;
		if (aiGetMaterialString(ai_material, AI_MATKEY_TEXTURE_DIFFUSE(0), &path) == AI_SUCCESS)
		{
			auto img_path = boost::filesystem::absolute(path.data, dir);
			material->diffuse_tex = LoadTexture(model, img_path.string());
		}
	}

	return material;
}

int AssimpHelper::LoadTexture(Model& model, const std::string& filepath)
{
	int idx = 0;
	for (auto& tex : model.textures)
	{
		if (tex.first == filepath) {
			return idx;
		}
		++idx;
	}

	int ret = model.textures.size();
	model.textures.push_back({ filepath, TextureLoader::LoadFromFile(filepath.c_str()) });
	return ret;
}

std::unique_ptr<SkeletalAnim::ModelExtend> AssimpHelper::LoadAnimation(const aiAnimation* ai_anim)
{
	auto ext = std::make_unique<SkeletalAnim::ModelExtend>();
	ext->name = ai_anim->mName.C_Str();
	ext->duration = static_cast<float>(ai_anim->mDuration);
	ext->ticks_per_second = static_cast<float>(ai_anim->mTicksPerSecond);
	if (ext->ticks_per_second == 1) {
		ext->ticks_per_second = 30;
	}
	for (int i = 0, n = ai_anim->mNumChannels; i < n; ++i) {
		auto& src = ai_anim->mChannels[i];
		ext->channels.push_back(LoadNodeAnim(src));
	}
	return ext;
}

std::unique_ptr<SkeletalAnim::NodeAnim> AssimpHelper::LoadNodeAnim(const aiNodeAnim* ai_node)
{
	auto node_anim = std::make_unique<SkeletalAnim::NodeAnim>();
	node_anim->name = ai_node->mNodeName.C_Str();

	node_anim->position_keys.reserve(ai_node->mNumPositionKeys);
	for (size_t i = 0; i < ai_node->mNumPositionKeys; ++i)
	{
		auto& src = ai_node->mPositionKeys[i];
		node_anim->position_keys.push_back({
			static_cast<float>(src.mTime), trans_ai_vector3d(src.mValue)
		});
	}

	node_anim->rotation_keys.reserve(ai_node->mNumRotationKeys);
	for (size_t i = 0; i < ai_node->mNumRotationKeys; ++i)
	{
		auto& src = ai_node->mRotationKeys[i];
		node_anim->rotation_keys.push_back({
			static_cast<float>(src.mTime), trans_ai_quaternion(src.mValue)
		});
	}

	node_anim->scaling_keys.reserve(ai_node->mNumScalingKeys);
	for (size_t i = 0; i < ai_node->mNumScalingKeys; ++i)
	{
		auto& src = ai_node->mScalingKeys[i];
		node_anim->scaling_keys.push_back({
			static_cast<float>(src.mTime), trans_ai_vector3d(src.mValue)
		});
	}

	return node_anim;
}

void AssimpHelper::CombineAABB(sm::cube& dst, const sm::cube& src, const sm::mat4& mat)
{
	dst.Combine(mat * sm::vec3(src.xmin, src.ymin, src.zmin));
	dst.Combine(mat * sm::vec3(src.xmin, src.ymax, src.zmin));
	dst.Combine(mat * sm::vec3(src.xmax, src.ymax, src.zmin));
	dst.Combine(mat * sm::vec3(src.xmax, src.ymin, src.zmin));
	dst.Combine(mat * sm::vec3(src.xmin, src.ymin, src.zmax));
	dst.Combine(mat * sm::vec3(src.xmin, src.ymax, src.zmax));
	dst.Combine(mat * sm::vec3(src.xmax, src.ymax, src.zmax));
	dst.Combine(mat * sm::vec3(src.xmax, src.ymin, src.zmax));
}

}