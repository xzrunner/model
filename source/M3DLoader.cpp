// This code from d3d12book https://github.com/d3dcoder/d3d12book

#include "model/M3DLoader.h"
#include "model/SkinnedData.h"
#include "model/Scene.h"
#include "model/typedef.h"
#include "model/Callback.h"
#include "model/EffectType.h"

#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

#include <boost/filesystem.hpp>

#include <fstream>

namespace
{

const float MODEL_SCALE = 0.1f;

}

namespace model
{

bool M3DLoader::Load(Scene& scene, const std::string& filepath)
{
	auto dir = boost::filesystem::path(filepath).parent_path().string();

	std::vector<M3DLoader::SkinnedVertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<M3DLoader::Subset> subsets;
	std::vector<M3DLoader::M3dMaterial> mats;
	SkinnedData skin_info;
	if (!M3DLoader::Load(filepath, vertices, indices, subsets, mats, skin_info)) {
		return false;
	}

	// aabb
	for (auto& v : vertices) {
		scene.aabb.Combine(v.Pos);
	}

	const int stride = sizeof(M3DLoader::SkinnedVertex) / sizeof(float);

	ur::RenderContext::VertexInfo vi;

	vi.vn = vertices.size() * stride;
	vi.vertices = &vertices[0].Pos.xyz[0];

	vi.in = indices.size();
	vi.indices = &indices[0];

	vi.va_list.push_back(ur::RenderContext::VertexAttribute(0, 3, stride, 0));  // pos
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(1, 3, stride, 3));  // normal
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(2, 2, stride, 6));  // texcoord
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(3, 3, stride, 8));  // tangent
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(4, 3, stride, 11)); // bone_weights
	vi.va_list.push_back(ur::RenderContext::VertexAttribute(5, 4, stride, 14)); // bone_indices

	//// material
	//scene.materials.emplace_back(std::make_unique<Scene::Material>());

	// mesh
	auto mesh = std::make_unique<Scene::Mesh>();
	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	mesh->effect = EFFECT_DEFAULT;
	int idx = 0;
	GD_ASSERT(subsets.size() == mats.size(), "err material");
	for (auto& sub : subsets)
	{
		//mesh->geometry.sub_geometries.insert({
		//	"sm_" + std::to_string(idx++),
		//	SubmeshGeometry(sub.FaceCount * 3, sub.FaceStart * 3)
		//});
		mesh->geometry.sub_geometries.emplace_back(
			sub.FaceCount * 3, sub.FaceStart * 3
		);

		auto& mat_src = mats[idx];

		// todo
		//Material mat_dst;
		//mat_dst.DiffuseAlbedo = mat_src.DiffuseAlbedo;
		//mat_dst.FresnelR0 = mat_src.FresnelR0;
		//mat_dst.Roughness = mat_src.Roughness;
		//auto img_path = boost::filesystem::absolute(mat_src.DiffuseMapName, dir);
		//mat_dst.texture = Callback::CreateImg(img_path.string());
		//mesh->materials.push_back(mat_dst);

		auto material = std::make_unique<Scene::Material>();
		material->diffuse_tex = scene.textures.size();
		auto img_path = boost::filesystem::absolute(mat_src.DiffuseMapName, dir);
		scene.textures.push_back({ filepath, Callback::CreateImg(img_path.string()) });
		scene.materials.push_back(std::move(material));

		mesh->geometry.sub_geometry_materials.push_back(idx);

		idx++;
	}
	scene.meshes.push_back(std::move(mesh));

	// node
	auto node = std::make_unique<Scene::Node>();
	node->meshes.emplace_back(0);
	scene.nodes.push_back(std::move(node));

	return true;
}

bool M3DLoader::Load(const std::string& filename,
					 std::vector<Vertex>& vertices,
					 std::vector<uint16_t>& indices,
					 std::vector<Subset>& subsets,
					 std::vector<M3dMaterial>& mats)
{
	std::ifstream fin(filename);

	uint32_t numMaterials = 0;
	uint32_t numVertices  = 0;
	uint32_t numTriangles = 0;
	uint32_t numBones     = 0;
	uint32_t numAnimationClips = 0;

	std::string ignore;

	if( fin )
	{
		fin >> ignore; // file header text
		fin >> ignore >> numMaterials;
		fin >> ignore >> numVertices;
		fin >> ignore >> numTriangles;
		fin >> ignore >> numBones;
		fin >> ignore >> numAnimationClips;

		ReadMaterials(fin, numMaterials, mats);
		ReadSubsetTable(fin, numMaterials, subsets);
	    ReadVertices(fin, numVertices, vertices);
	    ReadTriangles(fin, numTriangles, indices);

		return true;
	 }
    return false;
}

bool M3DLoader::Load(const std::string& filename,
					 std::vector<SkinnedVertex>& vertices,
					 std::vector<uint16_t>& indices,
					 std::vector<Subset>& subsets,
					 std::vector<M3dMaterial>& mats,
					 SkinnedData& skinInfo)
{
    std::ifstream fin(filename);

	//std::string str;
	//std::getline(fin, str);

	uint32_t numMaterials = 0;
	uint32_t numVertices  = 0;
	uint32_t numTriangles = 0;
	uint32_t numBones     = 0;
	uint32_t numAnimationClips = 0;

	std::string ignore;

	if( fin )
	{
		fin >> ignore; // file header text
		fin >> ignore >> numMaterials;
		fin >> ignore >> numVertices;
		fin >> ignore >> numTriangles;
		fin >> ignore >> numBones;
		fin >> ignore >> numAnimationClips;

		std::vector<sm::mat4> boneOffsets;
		std::vector<int> boneIndexToParentIndex;
		std::unordered_map<std::string, AnimationClip> animations;

		ReadMaterials(fin, numMaterials, mats);
		ReadSubsetTable(fin, numMaterials, subsets);
	    ReadSkinnedVertices(fin, numVertices, vertices);
	    ReadTriangles(fin, numTriangles, indices);
		ReadBoneOffsets(fin, numBones, boneOffsets);
	    ReadBoneHierarchy(fin, numBones, boneIndexToParentIndex);
	    ReadAnimationClips(fin, numBones, numAnimationClips, animations);

		skinInfo.Set(boneIndexToParentIndex, boneOffsets, animations);

	    return true;
	}
    return false;
}

void M3DLoader::ReadMaterials(std::ifstream& fin, uint32_t numMaterials, std::vector<M3dMaterial>& mats)
{
	 std::string ignore;
     mats.resize(numMaterials);

	 std::string diffuseMapName;
	 std::string normalMapName;

     fin >> ignore; // materials header text
	 for(uint32_t i = 0; i < numMaterials; ++i)
	 {
         fin >> ignore >> mats[i].Name;
		 fin >> ignore >> mats[i].DiffuseAlbedo.x  >> mats[i].DiffuseAlbedo.y  >> mats[i].DiffuseAlbedo.z;
		 fin >> ignore >> mats[i].FresnelR0.x >> mats[i].FresnelR0.y >> mats[i].FresnelR0.z;
         fin >> ignore >> mats[i].Roughness;
		 fin >> ignore >> mats[i].AlphaClip;
		 fin >> ignore >> mats[i].MaterialTypeName;
		 fin >> ignore >> mats[i].DiffuseMapName;
		 fin >> ignore >> mats[i].NormalMapName;
		}
}

void M3DLoader::ReadSubsetTable(std::ifstream& fin, uint32_t numSubsets, std::vector<Subset>& subsets)
{
    std::string ignore;
	subsets.resize(numSubsets);

	fin >> ignore; // subset header text
	for(uint32_t i = 0; i < numSubsets; ++i)
	{
        fin >> ignore >> subsets[i].Id;
		fin >> ignore >> subsets[i].VertexStart;
		fin >> ignore >> subsets[i].VertexCount;
		fin >> ignore >> subsets[i].FaceStart;
		fin >> ignore >> subsets[i].FaceCount;
    }
}

void M3DLoader::ReadVertices(std::ifstream& fin, uint32_t numVertices, std::vector<Vertex>& vertices)
{
	std::string ignore;
    vertices.resize(numVertices);

    fin >> ignore; // vertices header text
    for(uint32_t i = 0; i < numVertices; ++i)
    {
	    fin >> ignore >> vertices[i].Pos.x      >> vertices[i].Pos.y      >> vertices[i].Pos.z;
		fin >> ignore >> vertices[i].TangentU.x >> vertices[i].TangentU.y >> vertices[i].TangentU.z >> vertices[i].TangentU.w;
	    fin >> ignore >> vertices[i].Normal.x   >> vertices[i].Normal.y   >> vertices[i].Normal.z;
	    fin >> ignore >> vertices[i].TexC.x     >> vertices[i].TexC.y;

		vertices[i].Pos *= MODEL_SCALE;
    }
}

void M3DLoader::ReadSkinnedVertices(std::ifstream& fin, uint32_t numVertices, std::vector<SkinnedVertex>& vertices)
{
	std::string ignore;
    vertices.resize(numVertices);

    fin >> ignore; // vertices header text
	int boneIndices[4];
	float weights[4];
    for(uint32_t i = 0; i < numVertices; ++i)
    {
        float blah;
	    fin >> ignore >> vertices[i].Pos.x        >> vertices[i].Pos.y          >> vertices[i].Pos.z;
		fin >> ignore >> vertices[i].TangentU.x   >> vertices[i].TangentU.y     >> vertices[i].TangentU.z >> blah /*vertices[i].TangentU.w*/;
	    fin >> ignore >> vertices[i].Normal.x     >> vertices[i].Normal.y       >> vertices[i].Normal.z;
	    fin >> ignore >> vertices[i].TexC.x       >> vertices[i].TexC.y;
		fin >> ignore >> weights[0]     >> weights[1]     >> weights[2]     >> weights[3];
		fin >> ignore >> boneIndices[0] >> boneIndices[1] >> boneIndices[2] >> boneIndices[3];

		vertices[i].TexC.y = 1 - vertices[i].TexC.y;

		vertices[i].BoneWeights.x = weights[0];
		vertices[i].BoneWeights.y = weights[1];
		vertices[i].BoneWeights.z = weights[2];

		vertices[i].BoneIndices[0] = (uint8_t)boneIndices[0];
		vertices[i].BoneIndices[1] = (uint8_t)boneIndices[1];
		vertices[i].BoneIndices[2] = (uint8_t)boneIndices[2];
		vertices[i].BoneIndices[3] = (uint8_t)boneIndices[3];

		vertices[i].Pos *= MODEL_SCALE;
    }
}

void M3DLoader::ReadTriangles(std::ifstream& fin, uint32_t numTriangles, std::vector<uint16_t>& indices)
{
	std::string ignore;
    indices.resize(numTriangles*3);

    fin >> ignore; // triangles header text
    for(uint32_t i = 0; i < numTriangles; ++i)
    {
        fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
    }
}

void M3DLoader::ReadBoneOffsets(std::ifstream& fin, uint32_t numBones, std::vector<sm::mat4>& boneOffsets)
{
	std::string ignore;
    boneOffsets.resize(numBones);

    fin >> ignore; // BoneOffsets header text
    for(uint32_t i = 0; i < numBones; ++i)
    {
        fin >> ignore >>
            boneOffsets[i].c[0][0] >> boneOffsets[i].c[0][1] >> boneOffsets[i].c[0][2] >> boneOffsets[i].c[0][3] >>
            boneOffsets[i].c[1][0] >> boneOffsets[i].c[1][1] >> boneOffsets[i].c[1][2] >> boneOffsets[i].c[1][3] >>
            boneOffsets[i].c[2][0] >> boneOffsets[i].c[2][1] >> boneOffsets[i].c[2][2] >> boneOffsets[i].c[2][3] >>
            boneOffsets[i].c[3][0] >> boneOffsets[i].c[3][1] >> boneOffsets[i].c[3][2] >> boneOffsets[i].c[3][3];
    }
}

void M3DLoader::ReadBoneHierarchy(std::ifstream& fin, uint32_t numBones, std::vector<int>& boneIndexToParentIndex)
{
	std::string ignore;
    boneIndexToParentIndex.resize(numBones);

    fin >> ignore; // BoneHierarchy header text
	for(uint32_t i = 0; i < numBones; ++i)
	{
	    fin >> ignore >> boneIndexToParentIndex[i];
	}
}

void M3DLoader::ReadAnimationClips(std::ifstream& fin, uint32_t numBones, uint32_t numAnimationClips,
								   std::unordered_map<std::string, AnimationClip>& animations)
{
	std::string ignore;
    fin >> ignore; // AnimationClips header text
    for(uint32_t clipIndex = 0; clipIndex < numAnimationClips; ++clipIndex)
    {
        std::string clipName;
        fin >> ignore >> clipName;
        fin >> ignore; // {

		AnimationClip clip;
		clip.BoneAnimations.resize(numBones);

        for(uint32_t boneIndex = 0; boneIndex < numBones; ++boneIndex)
        {
            ReadBoneKeyframes(fin, numBones, clip.BoneAnimations[boneIndex]);
        }
        fin >> ignore; // }

        animations[clipName] = clip;
    }
}

void M3DLoader::ReadBoneKeyframes(std::ifstream& fin, uint32_t numBones, BoneAnimation& boneAnimation)
{
	std::string ignore;
    uint32_t numKeyframes = 0;
    fin >> ignore >> ignore >> numKeyframes;
    fin >> ignore; // {

    boneAnimation.Keyframes.resize(numKeyframes);
    for(uint32_t i = 0; i < numKeyframes; ++i)
    {
        float t    = 0.0f;
        sm::vec3 p(0.0f, 0.0f, 0.0f);
		sm::vec3 s(1.0f, 1.0f, 1.0f);
		sm::vec4 q(0.0f, 0.0f, 0.0f, 1.0f);
        fin >> ignore >> t;
        fin >> ignore >> p.x >> p.y >> p.z;
        fin >> ignore >> s.x >> s.y >> s.z;
        fin >> ignore >> q.x >> q.y >> q.z >> q.w;

	    boneAnimation.Keyframes[i].TimePos      = t;
        boneAnimation.Keyframes[i].Translation  = p;
	    boneAnimation.Keyframes[i].Scale        = s;
	    boneAnimation.Keyframes[i].RotationQuat = q;
    }

    fin >> ignore; // }
}

}