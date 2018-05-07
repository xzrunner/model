#include "model/AssimpHelper.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/Callback.h"

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

bool AssimpHelper::Load(Model& model, const std::string& filepath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath.c_str(),
		ppsteps | /* configurable pp steps */
		aiProcess_GenSmoothNormals		   | // generate smooth normal vectors if not existing
		aiProcess_SplitLargeMeshes         | // split large, unrenderable meshes into submeshes
		aiProcess_Triangulate			   | // triangulate polygons with more than 3 edges
		aiProcess_ConvertToLeftHanded	   | // convert everything to D3D left handed space
		aiProcess_SortByPType                // make 'clean' meshes which consist of a single typ of primitives
		);

	if (!scene) {
		return NULL;
	}

	auto dir = boost::filesystem::path(filepath).parent_path().string();
	pt3::AABB aabb;
	LoadNode(scene, scene->mRootNode, model, dir, aabb);
	model.SetAABB(aabb);

	// todo: load lights and cameras

	return true;
}

void AssimpHelper::LoadNode(const aiScene* ai_scene, const aiNode* ai_node,
							Model& model, const std::string& dir, pt3::AABB& aabb)
{
	if (ai_node->mNumChildren)
	{
		for (size_t i = 0; i < ai_node->mNumChildren; ++i) {
			LoadNode(ai_scene, ai_node->mChildren[i], model, dir, aabb);
		}
	}
	else
	{
		if (ai_node->mNumMeshes)
		{
			sm::mat4 trans_mat;
			const aiNode* node = ai_node;
			while (node) {
				sm::mat4 node_mat;
				node_mat.x[0] = node->mTransformation.a1;
				node_mat.x[1] = node->mTransformation.b1;
				node_mat.x[2] = node->mTransformation.c1;
				node_mat.x[3] = node->mTransformation.d1;
				node_mat.x[4] = node->mTransformation.a2;
				node_mat.x[5] = node->mTransformation.b2;
				node_mat.x[6] = node->mTransformation.c2;
				node_mat.x[7] = node->mTransformation.d2;
				node_mat.x[8] = node->mTransformation.a3;
				node_mat.x[9] = node->mTransformation.b3;
				node_mat.x[10] = node->mTransformation.c3;
				node_mat.x[11] = node->mTransformation.d3;
				node_mat.x[12] = node->mTransformation.a4;
				node_mat.x[13] = node->mTransformation.b4;
				node_mat.x[14] = node->mTransformation.c4;
				node_mat.x[15] = node->mTransformation.d4;

				trans_mat = node_mat * trans_mat;

				node = node->mParent;
			}

			for (size_t i = 0; i < ai_node->mNumMeshes; ++i)
			{
				const aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
				const aiMaterial* ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

				auto mesh = LoadMesh(ai_mesh, ai_material, dir, trans_mat, aabb);
				model.AddMesh(mesh);
			}
		}
	}
}

std::unique_ptr<Mesh> AssimpHelper::LoadMesh(const aiMesh* ai_mesh, const aiMaterial* ai_material,
	                                         const std::string& dir, const sm::mat4& trans, pt3::AABB& aabb)
{
	auto mesh = std::make_unique<Mesh>();

	LoadMaterial(ai_mesh, ai_material, *mesh, dir);

	int vertex_type = 0;
	int floats_per_vertex = 3;
	bool has_normal = ai_mesh->HasNormals();
	if (has_normal) {
		floats_per_vertex += 3;
		vertex_type |= VERTEX_FLAG_NORMALS;
	}
	bool has_texcoord = ai_mesh->HasTextureCoords(0);
	if (has_texcoord) {
		floats_per_vertex += 2;
		vertex_type |= VERTEX_FLAG_TEXCOORDS;
	}

	std::vector<float> vertices;
	vertices.reserve(floats_per_vertex * ai_mesh->mNumVertices);
	for (size_t i = 0; i < ai_mesh->mNumVertices; ++i)
	{
		const aiVector3D& p = ai_mesh->mVertices[i];

		sm::vec3 p_trans = trans * sm::vec3(p.x, p.y, p.z);

		vertices.push_back(p_trans.x);
		vertices.push_back(p_trans.y);
		vertices.push_back(p_trans.z);
		aabb.Combine(p_trans);

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

void AssimpHelper::LoadMaterial(const aiMesh* ai_mesh, const aiMaterial* ai_material, Mesh& mesh, const std::string& dir)
{
	model::MaterialOld material;

	aiColor4D col;

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_DIFFUSE, &col) == AI_SUCCESS) {
		material.diffuse.x = col.r;
		material.diffuse.y = col.g;
		material.diffuse.z = col.b;
	} else {
		material.diffuse.x = material.diffuse.y = material.diffuse.z = 1;
	}

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_SPECULAR, &col) == AI_SUCCESS) {
		material.specular.x = col.r;
		material.specular.y = col.g;
		material.specular.z = col.b;
	} else {
		material.specular.x = material.specular.y = material.specular.z = 1;
	}

	if (aiGetMaterialColor(ai_material, AI_MATKEY_COLOR_AMBIENT, &col) == AI_SUCCESS) {
		material.ambient.x = col.r;
		material.ambient.y = col.g;
		material.ambient.z = col.b;
	} else {
		material.ambient.x = material.ambient.y = material.ambient.z = 0.04f;
	}
	material.shininess = 50;

	material.texture = nullptr;
	if (ai_mesh->mTextureCoords[0])
	{
		aiString path;
		if (aiGetMaterialString(ai_material, AI_MATKEY_TEXTURE_DIFFUSE(0), &path) == AI_SUCCESS)
		{
			auto img_path = boost::filesystem::absolute(path.data, dir);
			material.texture = Callback::CreateImg(img_path.string());
		}
	}

	mesh.old_materials.push_back(material);
}

}