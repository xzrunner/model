#include "model/AssimpHelper.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/TextureLoader.h"

#include <SM_Matrix.h>
#include <SM_Cube.h>
#include <unirender2/Device.h>
#include <unirender2/VertexArray.h>
#include <unirender2/IndexBuffer.h>
#include <unirender2/VertexBuffer.h>
#include <unirender2/VertexBufferAttribute.h>

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

    //sm::vec3 trans, rotate, scale;
    //mat.Decompose(trans, rotate, scale);

    //auto mat2 = mat;
    //mat2.x[12] = 0;
    //mat2.x[13] = 0;
    //mat2.x[14] = 0;
    //sm::vec3 x = mat2 * sm::vec3(1, 0, 0);
    //sm::vec3 y = mat2 * sm::vec3(0, 1, 0);
    //sm::vec3 z = mat2 * sm::vec3(0, 0, 1);
    //printf("x %f %f %f, y %f %f %f, z %f %f %f\n",
    //    x.x, x.y, x.z, y.x, y.y, y.z, z.x, z.y, z.z);

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

bool     AssimpHelper::m_load_raw_data = false;
uint32_t AssimpHelper::m_vert_color = 0;

bool AssimpHelper::Load(const ur2::Device& dev, Model& model, const std::string& filepath, float scale)
{
	Assimp::Importer importer;
    importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, scale);
	const aiScene* ai_scene = importer.ReadFile(filepath.c_str(),
		//ppsteps | /* configurable pp steps */
		//aiProcess_GenSmoothNormals		   | // generate smooth normal vectors if not existing
		//aiProcess_SplitLargeMeshes         | // split large, unrenderable meshes into submeshes
		//aiProcess_Triangulate			   | // triangulate polygons with more than 3 edges
		//aiProcess_ConvertToLeftHanded	   | // convert everything to D3D left handed space
		//aiProcess_SortByPType                // make 'clean' meshes which consist of a single typ of primitives

        //aiProcess_SplitByBoneCount |

        aiProcess_Triangulate |

        aiProcess_GlobalScale |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ValidateDataStructure

		);

	if (!ai_scene) {
		return NULL;
	}

	// material
	auto dir = boost::filesystem::path(filepath).parent_path().string();
	model.materials.reserve(ai_scene->mNumMaterials);
	for (size_t i = 0; i < ai_scene->mNumMaterials; ++i)
	{
		auto src = ai_scene->mMaterials[i];
		model.materials.push_back(LoadMaterial(dev, src, model, dir));
	}

    ////
    //for (size_t i = 0; i < ai_scene->mNumMeshes; ++i)
    //{
    //    auto ai_mesh = ai_scene->mMeshes[i];
    //    printf("num_bones %d\n", ai_mesh->mNumBones);
    //    for (size_t i = 0; i < ai_mesh->mNumBones; ++i)
    //    {
    //        auto src = ai_mesh->mBones[i];
    //        printf("%s\n", src->mName.C_Str());
    //        auto offset_trans = trans_ai_mat(src->mOffsetMatrix);
    //    }
    //}

	// mesh
	std::vector<sm::cube> meshes_aabb;
	meshes_aabb.reserve(ai_scene->mNumMeshes);
	model.meshes.reserve(ai_scene->mNumMeshes);
	for (size_t i = 0; i < ai_scene->mNumMeshes; ++i)
	{
		auto src = ai_scene->mMeshes[i];
		sm::cube aabb;
		model.meshes.push_back(LoadMesh(dev, model.materials, src, aabb));
		meshes_aabb.push_back(aabb);
	}

	// only meshes
	if (ai_scene->mRootNode->mNumChildren == 0)
	{
		for (auto& ab : meshes_aabb) {
			model.aabb.Combine(ab);
		}
	}
	// skeletal anim
	else
	{
		auto ext = std::make_unique<SkeletalAnim>();

		// node
		{
			std::vector<std::unique_ptr<SkeletalAnim::Node>> nodes;
			LoadNode(ai_scene, ai_scene->mRootNode, model, nodes, meshes_aabb, sm::mat4());
			ext->SetNodes(nodes);
		}

		// bone
		auto& nodes = ext->GetNodes();
		for (auto& mesh : model.meshes) {
			for (auto& bone : mesh->geometry.bones) {
				bone.node = -1;
				for (int i = 0, n = nodes.size(); i < n; ++i) {
					if (nodes[i]->name == bone.name) {
						bone.node = i;
						break;
					}
				}
			}
		}

		// animation
		std::vector<std::unique_ptr<SkeletalAnim::ModelExtend>> anims;
		anims.reserve(ai_scene->mNumAnimations);
		for (size_t i = 0; i < ai_scene->mNumAnimations; ++i) {
			auto src = ai_scene->mAnimations[i];
			anims.push_back(LoadAnimation(src));
		}
		ext->SetAnims(anims);

		model.ext = std::move(ext);
	}

	// todo: load lights and cameras

	// todo
	if (boost::filesystem::extension(filepath) == ".X") {
		model.anim_speed = 100;
	}

	model.scale = 1.0f;

	return true;
}

bool AssimpHelper::Load(std::vector<std::unique_ptr<MeshRawData>>& meshes, const std::string& filepath)
{
	Assimp::Importer importer;
	const aiScene* ai_scene = importer.ReadFile(filepath.c_str(),
        aiProcess_Triangulate |
        aiProcess_GlobalScale |
        aiProcess_GenSmoothNormals |
        aiProcess_ConvertToLeftHanded |
        aiProcess_JoinIdenticalVertices |
        aiProcess_ValidateDataStructure
	);

	if (!ai_scene) {
		return false;
	}

    meshes.clear();
	for (size_t i = 0; i < ai_scene->mNumMeshes; ++i) {
        meshes.push_back(LoadMeshRawData(ai_scene->mMeshes[i]));
	}

	return true;
}

int AssimpHelper::LoadNode(const aiScene* ai_scene, const aiNode* ai_node, Model& model,
	                       std::vector<std::unique_ptr<SkeletalAnim::Node>>& nodes,
	                       const std::vector<sm::cube>& meshes_aabb, const sm::mat4& mat)
{
	auto node = std::make_unique<SkeletalAnim::Node>();
	auto node_raw = node.get();

	node_raw->name = ai_node->mName.C_Str();

	int node_id = nodes.size();
	nodes.push_back(std::move(node));

	node_raw->local_trans = trans_ai_mat(ai_node->mTransformation);

	auto child_mat = mat * node_raw->local_trans;   // mat mul

	if (ai_node->mNumChildren)
	{
		for (size_t i = 0; i < ai_node->mNumChildren; ++i)
		{
			int child = LoadNode(ai_scene, ai_node->mChildren[i], model, nodes, meshes_aabb, child_mat);
			node_raw->children.push_back(child);

			auto node = nodes[child].get();
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
                if (model.meshes[mesh]->name.empty()) {
                    model.meshes[mesh]->name = ai_node->mName.C_Str();
                } else {
                    //if (model.meshes[mesh]->name != ai_node->mName.C_Str()) {
                    //    assert(0);
                    //}
                    //if (model.meshes[mesh]->name.find(ai_node->mName.C_Str()) == std::string::npos) {
                    //    assert(0);
                    //}
                }
				CombineAABB(model.aabb, meshes_aabb[mesh], child_mat);
			}
		}
	}

	return node_id;
}

std::unique_ptr<Model::Mesh>
AssimpHelper::LoadMesh(const ur2::Device& dev, const std::vector<std::unique_ptr<Model::Material>>& materials,
                       const aiMesh* ai_mesh, sm::cube& aabb)
{
	auto mesh = std::make_unique<Model::Mesh>();

    mesh->name = ai_mesh->mName.C_Str();

	mesh->material = ai_mesh->mMaterialIndex;

	bool has_mat_tex = false;
	if (mesh->material >= 0 && mesh->material < static_cast<int>(materials.size())) {
		if (materials[mesh->material]->diffuse_tex >= 0) {
			has_mat_tex = true;
		}
	}

	int floats_per_vertex = 3;

	bool has_normal = ai_mesh->HasNormals();
	if (has_normal) {
		floats_per_vertex += 3;
	}

	bool has_texcoord = ai_mesh->HasTextureCoords(0);
	if (has_texcoord) {
		floats_per_vertex += 2;
	}

	const bool has_color = m_vert_color != 0;
	if (has_color) {
		floats_per_vertex += 1;
	}

	bool has_skinned = ai_mesh->HasBones();
	if (has_skinned) {
		floats_per_vertex += 2;
	}

	std::vector<std::vector<std::pair<int, float>>> weights_per_vertex(ai_mesh->mNumVertices);
	for (unsigned int i = 0; i < ai_mesh->mNumBones; ++i)
	{
		auto bone = ai_mesh->mBones[i];
		for (unsigned int j = 0; j < bone->mNumWeights; ++j) {
			weights_per_vertex[bone->mWeights[j].mVertexId].push_back({ i, bone->mWeights[j].mWeight });
		}
	}

    // fixme: to protect
    for (auto& w : weights_per_vertex) {
        if (w.empty()) {
            w.push_back({ 0, 1.0f });
        }
    }

	uint8_t* buf = new uint8_t[ai_mesh->mNumVertices * floats_per_vertex * sizeof(float)];
	uint8_t* ptr = buf;
	for (size_t i = 0; i < ai_mesh->mNumVertices; ++i)
	{
		const aiVector3D& p = ai_mesh->mVertices[i];

		sm::vec3 p_trans(p.x, p.y, p.z);
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
            float x = t.x;
            if (x > 1) {
                x -= std::floor(x);
            }
			memcpy(ptr, &x, sizeof(float));
			ptr += sizeof(float);
			float y = 1 - t.y;
            if (y > 1) {
                y -= std::floor(y);
            }
			memcpy(ptr, &y, sizeof(float));
			ptr += sizeof(float);
		}
		if (has_color)
		{
			memcpy(ptr, &m_vert_color, sizeof(uint32_t));
			ptr += sizeof(uint32_t);
		}
		if (has_skinned)
		{
			unsigned char indices[4] = { 0, 0, 0, 0 };
			unsigned char weights[4] = { 0, 0, 0, 0 };
			for (int j = 0, m = std::min(static_cast<int>(weights_per_vertex[i].size()), 4); j < m; ++j)
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

    auto va = dev.CreateVertexArray();

    auto ibuf_sz = sizeof(uint16_t) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(ur2::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf_sz = sizeof(float) * floats_per_vertex * ai_mesh->mNumVertices;
    auto vbuf = dev.CreateVertexBuffer(ur2::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(buf, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur2::VertexBufferAttribute>> vbuf_attrs;

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
	// color
	if (has_color) {
		stride += 4;
	}
	// skinned
	if (has_skinned) {
		stride += 4 + 4;
	}

	int offset = 0;
	// pos
    vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, offset, stride));
	offset += 4 * 3;
	// normal
	if (has_normal)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Float, 3, offset, stride));
		offset += 4 * 3;
	}
	// texcoord
	if (has_texcoord)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::Float, 2, offset, stride));
		offset += 4 * 2;
	}
	// color
	if (has_color)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_COLOR;
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::UnsignedByte, 4, offset, stride));
		offset += 4;
	}
	// skinned
	if (has_skinned)
	{
		mesh->geometry.vertex_type |= VERTEX_FLAG_SKINNED;
        // blend_indices
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::UnsignedByte, 4, offset, stride));
		offset += 4;
        // blend_weights
        vbuf_attrs.push_back(std::make_shared<ur2::VertexBufferAttribute>(
            ur2::ComponentDataType::UnsignedByte, 4, offset, stride));
		offset += 4;
	}

    va->SetVertexBufferAttrs(vbuf_attrs);

    mesh->geometry.vertex_array = va;
//	mesh->geometry.sub_geometries.insert({ "default", SubmeshGeometry(vi.in, 0) });
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, indices.size(), 0));
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

//	delete[] buf;
    mesh->geometry.n_vert = ai_mesh->mNumVertices;
    mesh->geometry.n_poly = ai_mesh->mNumFaces;
    mesh->geometry.vert_stride = stride;
    mesh->geometry.vert_buf = buf;

	if (m_load_raw_data) {
		mesh->geometry.raw_data = LoadMeshRawData(ai_mesh);
        mesh->geometry.raw_data->weights_per_vertex = weights_per_vertex;

        assert(weights_per_vertex.size() == mesh->geometry.raw_data->vertices.size());
	}

	return mesh;
}

std::unique_ptr<MeshRawData> AssimpHelper::LoadMeshRawData(const aiMesh* ai_mesh)
{
	auto rd = std::make_unique<model::MeshRawData>();

	rd->vertices.reserve(ai_mesh->mNumVertices);
	for (size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
		auto& p = ai_mesh->mVertices[i];
		rd->vertices.emplace_back(p.x, p.y, p.z);
	}

	if (ai_mesh->HasNormals())
	{
		rd->normals.reserve(ai_mesh->mNumVertices);
		for (size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
			auto& p = ai_mesh->mNormals[i];
			rd->normals.emplace_back(p.x, p.y, p.z);
		}
	}

    if (ai_mesh->HasTextureCoords(0))
    {
        rd->texcoords.reserve(ai_mesh->mNumVertices);
        for (size_t i = 0; i < ai_mesh->mNumVertices; ++i) {
            auto& p = ai_mesh->mTextureCoords[0][i];
            rd->texcoords.emplace_back(p.x, p.y);
        }
    }

	rd->faces.reserve(ai_mesh->mNumFaces);
	for (size_t i = 0; i < ai_mesh->mNumFaces; ++i)
	{
		const aiFace& ai_face = ai_mesh->mFaces[i];
		std::vector<int> face;
		face.reserve(ai_face.mNumIndices);
		for (size_t j = 0; j < ai_face.mNumIndices; ++j) {
			face.push_back(ai_face.mIndices[j]);
		}
		rd->faces.push_back(face);
	}

	return rd;
}

std::unique_ptr<Model::Material>
AssimpHelper::LoadMaterial(const ur2::Device& dev, const aiMaterial* ai_material,
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
	// todo:
	if (material->ambient == sm::vec3(1, 1, 1)) {
		material->ambient.Set(0.04f, 0.04f, 0.04f);
	}
	material->shininess = 50;

	material->diffuse_tex = -1;
//	if (ai_mesh->mTextureCoords[0])
	{
		aiString path;
		if (aiGetMaterialString(ai_material, AI_MATKEY_TEXTURE_DIFFUSE(0), &path) == AI_SUCCESS)
		{
			auto img_path = boost::filesystem::absolute(path.data, dir);
			material->diffuse_tex = LoadTexture(dev, model, img_path.string());
		}
	}

	return material;
}

int AssimpHelper::LoadTexture(const ur2::Device& dev, Model& model, const std::string& filepath)
{
	int idx = 0;
	for (auto& tex : model.textures)
	{
		if (tex.first == filepath) {
			return idx;
		}
		++idx;
	}

    auto tex = TextureLoader::LoadFromFile(dev, filepath.c_str());
    if (!tex) {
        return -1;
    }

	int ret = model.textures.size();
	model.textures.push_back({ filepath, tex });
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