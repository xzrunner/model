#include "model/AssimpHelper.h"
#include "model/typedef.h"
#include "model/Callback.h"
#include "model/Scene.h"
#include "model/EffectType.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

#include <SM_Matrix.h>
#include <painting3/AABB.h>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <boost/filesystem.hpp>

namespace model
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

bool AssimpHelper::Load(Scene& scene, const std::string& filepath)
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

	// load material
	auto dir = boost::filesystem::path(filepath).parent_path().string();
	scene.materials.reserve(ai_scene->mNumMaterials);
	for (size_t i = 0; i < ai_scene->mNumMaterials; ++i)
	{
		auto src = ai_scene->mMaterials[i];
		scene.materials.push_back(LoadMaterial(src, scene, dir));
	}

	// load mesh
	scene.meshes.reserve(ai_scene->mNumMeshes);
	for (size_t i = 0; i < ai_scene->mNumMeshes; ++i)
	{
		auto src = ai_scene->mMeshes[i];
		scene.meshes.push_back(LoadMesh(src));
	}

//	pt3::AABB aabb;
	LoadNode(ai_scene, ai_scene->mRootNode, scene);
//	scene.aabb = aabb;

	// todo: load lights and cameras

	return true;
}

int AssimpHelper::LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Scene& scene)
{
	auto node = std::make_unique<Scene::Node>();
	auto node_raw = node.get();

	node_raw->name = ai_node->mName.C_Str();

	int node_id = scene.nodes.size();
	scene.nodes.push_back(std::move(node));

	sm::mat4 node_mat;
	node_mat.x[0] = ai_node->mTransformation.a1;
	node_mat.x[1] = ai_node->mTransformation.b1;
	node_mat.x[2] = ai_node->mTransformation.c1;
	node_mat.x[3] = ai_node->mTransformation.d1;
	node_mat.x[4] = ai_node->mTransformation.a2;
	node_mat.x[5] = ai_node->mTransformation.b2;
	node_mat.x[6] = ai_node->mTransformation.c2;
	node_mat.x[7] = ai_node->mTransformation.d2;
	node_mat.x[8] = ai_node->mTransformation.a3;
	node_mat.x[9] = ai_node->mTransformation.b3;
	node_mat.x[10] = ai_node->mTransformation.c3;
	node_mat.x[11] = ai_node->mTransformation.d3;
	node_mat.x[12] = ai_node->mTransformation.a4;
	node_mat.x[13] = ai_node->mTransformation.b4;
	node_mat.x[14] = ai_node->mTransformation.c4;
	node_mat.x[15] = ai_node->mTransformation.d4;
	node_raw->local_mat = node_mat;

	if (ai_node->mNumChildren)
	{
		for (size_t i = 0; i < ai_node->mNumChildren; ++i)
		{
			int child = LoadNode(ai_scene, ai_node->mChildren[i], scene);
			node_raw->children.push_back(child);

			assert(scene.nodes[child]->parent == -1);
			scene.nodes[child]->parent = node_id;
		}
	}
	else
	{
		if (ai_node->mNumMeshes)
		{
			//sm::mat4 trans_mat;
			//const aiNode* node = ai_node;
			//while (node) {
			//	sm::mat4 node_mat;
			//	node_mat.x[0] = ai_node->mTransformation.a1;
			//	node_mat.x[1] = ai_node->mTransformation.b1;
			//	node_mat.x[2] = ai_node->mTransformation.c1;
			//	node_mat.x[3] = ai_node->mTransformation.d1;
			//	node_mat.x[4] = ai_node->mTransformation.a2;
			//	node_mat.x[5] = ai_node->mTransformation.b2;
			//	node_mat.x[6] = ai_node->mTransformation.c2;
			//	node_mat.x[7] = ai_node->mTransformation.d2;
			//	node_mat.x[8] = ai_node->mTransformation.a3;
			//	node_mat.x[9] = ai_node->mTransformation.b3;
			//	node_mat.x[10] = ai_node->mTransformation.c3;
			//	node_mat.x[11] = ai_node->mTransformation.d3;
			//	node_mat.x[12] = ai_node->mTransformation.a4;
			//	node_mat.x[13] = ai_node->mTransformation.b4;
			//	node_mat.x[14] = ai_node->mTransformation.c4;
			//	node_mat.x[15] = ai_node->mTransformation.d4;

			//	trans_mat = node_mat * trans_mat;

			//	node = node_raw->mParent;
			//}



			//for (size_t i = 0; i < ai_node->mNumMeshes; ++i)
			//{
			//	const aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
			//	const aiMaterial* ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

			//	auto mesh = LoadMesh(ai_mesh, trans_mat, aabb);
			//	scene.meshes.push_back(std::move(mesh));
			//}

			node_raw->meshes.reserve(ai_node->mNumMeshes);
			for (size_t i = 0; i < ai_node->mNumMeshes; ++i) {
				node_raw->meshes.push_back(ai_node->mMeshes[i]);
			}
		}
	}

	return node_id;
}

std::unique_ptr<Scene::Mesh> AssimpHelper::LoadMesh(const aiMesh* ai_mesh/*, const sm::mat4& trans, pt3::AABB& aabb*/)
{
	auto mesh = std::make_unique<Scene::Mesh>();

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

	std::vector<float> vertices;
	vertices.reserve(floats_per_vertex * ai_mesh->mNumVertices);
	for (size_t i = 0; i < ai_mesh->mNumVertices; ++i)
	{
		const aiVector3D& p = ai_mesh->mVertices[i];

		//sm::vec3 p_trans = trans * sm::vec3(p.x, p.y, p.z);
		sm::vec3 p_trans(p.x, p.y, p.z);

		vertices.push_back(p_trans.x);
		vertices.push_back(p_trans.y);
		vertices.push_back(p_trans.z);
		//aabb.Combine(p_trans);

		if (has_normal) {
			const aiVector3D& n = ai_mesh->mNormals[i];
			vertices.push_back(n.x);
			vertices.push_back(n.y);
			vertices.push_back(n.z);
		}
		if (has_texcoord) {
			const aiVector3D& t = ai_mesh->mTextureCoords[0][i];
			vertices.push_back(t.x);
			vertices.push_back(1 - t.y);
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

	vi.vn = vertices.size();
	vi.vertices = &vertices[0];
	vi.in = indices.size();
	vi.indices = &indices[0];

	size_t stride = 3;
	if (has_normal) {
		stride += 3;
	}
	if (has_texcoord) {
		stride += 2;
	}
	size_t index = 0;
	size_t offset = 0;
	// pos
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(index++, 3, stride, offset));
	offset += 3;
	// normal
	if (has_normal)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
		vi.va_list.push_back(ur::RenderContext::VertexAttribute(index++, 3, stride, offset));
		offset += 3;
	}
	// texcoord
	if (has_texcoord)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
		vi.va_list.push_back(ur::RenderContext::VertexAttribute(index++, 2, stride, offset));
		offset += 2;
	}

	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(vi.in, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);

	return mesh;
}

std::unique_ptr<Scene::Material> AssimpHelper::LoadMaterial(const aiMaterial* ai_material,
	                                                        Scene& scene, const std::string& dir)
{
	auto material = std::make_unique<Scene::Material>();

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
			material->diffuse_tex = LoadTexture(scene, img_path.string());
		}
	}

	return material;
}

int AssimpHelper::LoadTexture(Scene& scene, const std::string& filepath)
{
	int idx = 0;
	for (auto& tex : scene.textures)
	{
		if (tex.first == filepath) {
			return idx;
		}
		++idx;
	}

	int ret = scene.textures.size();
	scene.textures.push_back({ filepath, Callback::CreateImg(filepath) });
	return ret;
}

}