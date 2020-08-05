// This code from d3d12book https://github.com/d3dcoder/d3d12book

#include "model/M3dLoader.h"
#include "model/SkinnedData.h"
#include "model/Model.h"
#include "model/typedef.h"
#include "model/TextureLoader.h"

#include <guard/check.h>
#include <unirender/Device.h>
#include <unirender/VertexArray.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexInputAttribute.h>

#include <boost/filesystem.hpp>

#include <fstream>

namespace
{

const float MODEL_SCALE = 0.1f;

}

namespace model
{

bool M3dLoader::Load(const ur::Device& dev, Model& model, const std::string& filepath)
{
	auto dir = boost::filesystem::path(filepath).parent_path().string();

	std::vector<M3dLoader::SkinnedVertex> vertices;
	std::vector<uint16_t> indices;
	std::vector<M3dLoader::Subset> subsets;
	std::vector<M3dLoader::M3dMaterial> mats;
	//SkinnedData skin_info;
	//if (!M3dLoader::Load(filepath, vertices, indices, subsets, mats, skin_info)) {
	//	return false;
	//}

	// aabb
	for (auto& v : vertices) {
		model.aabb.Combine(v.Pos);
	}

	const int stride = sizeof(M3dLoader::SkinnedVertex) / sizeof(float);

    auto va = dev.CreateVertexArray();

    auto ibuf_sz = sizeof(uint16_t) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf_sz = sizeof(M3dLoader::SkinnedVertex) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(&vertices[0].Pos.xyz[0], vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs(6);
    // pos
    vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
        0, ur::ComponentDataType::Float, 3, 0, 52);
    // normal
    vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
        1, ur::ComponentDataType::Float, 3, 12, 52);
    // texcoord
    vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
        2, ur::ComponentDataType::Float, 2, 24, 52);
    // tangent
    vbuf_attrs[3] = std::make_shared<ur::VertexInputAttribute>(
        3, ur::ComponentDataType::Float, 3, 32, 52);
    // bone_weights
    vbuf_attrs[4] = std::make_shared<ur::VertexInputAttribute>(
        4, ur::ComponentDataType::UnsignedByte, 1, 44, 52);
    // bone_indices
    vbuf_attrs[5] = std::make_shared<ur::VertexInputAttribute>(
        5, ur::ComponentDataType::UnsignedByte, 1, 48, 52);
    va->SetVertexBufferAttrs(vbuf_attrs);

	//// material
	//model.materials.emplace_back(std::make_unique<Model::Material>());

	// mesh
	auto mesh = std::make_unique<Model::Mesh>();
    mesh->geometry.vertex_array = va;
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;
	int idx = 0;
	GD_ASSERT(subsets.size() == mats.size(), "err material");
	for (auto& sub : subsets)
	{
		//mesh->geometry.sub_geometries.insert({
		//	"sm_" + std::to_string(idx++),
		//	SubmeshGeometry(sub.FaceCount * 3, sub.FaceStart * 3)
		//});
		mesh->geometry.sub_geometries.emplace_back(
			true, sub.FaceCount * 3, sub.FaceStart * 3
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

		auto material = std::make_unique<Model::Material>();
		material->diffuse_tex = model.textures.size();
		auto img_path = boost::filesystem::absolute(mat_src.DiffuseMapName, dir);
		auto tex = TextureLoader::LoadFromFile(dev, img_path.string().c_str());
		model.textures.push_back({ img_path.string(), std::move(tex) });
		model.materials.push_back(std::move(material));

		mesh->geometry.sub_geometry_materials.push_back(idx);

		idx++;
	}
	model.meshes.push_back(std::move(mesh));

	return true;
}

bool M3dLoader::Load(const std::string& filename,
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

bool M3dLoader::Load(const std::string& filename,
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

void M3dLoader::ReadMaterials(std::ifstream& fin, uint32_t numMaterials, std::vector<M3dMaterial>& mats)
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

void M3dLoader::ReadSubsetTable(std::ifstream& fin, uint32_t numSubsets, std::vector<Subset>& subsets)
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

void M3dLoader::ReadVertices(std::ifstream& fin, uint32_t numVertices, std::vector<Vertex>& vertices)
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

void M3dLoader::ReadSkinnedVertices(std::ifstream& fin, uint32_t numVertices, std::vector<SkinnedVertex>& vertices)
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

void M3dLoader::ReadTriangles(std::ifstream& fin, uint32_t numTriangles, std::vector<uint16_t>& indices)
{
	std::string ignore;
    indices.resize(numTriangles*3);

    fin >> ignore; // triangles header text
    for(uint32_t i = 0; i < numTriangles; ++i)
    {
        fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
    }
}

void M3dLoader::ReadBoneOffsets(std::ifstream& fin, uint32_t numBones, std::vector<sm::mat4>& boneOffsets)
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

void M3dLoader::ReadBoneHierarchy(std::ifstream& fin, uint32_t numBones, std::vector<int>& boneIndexToParentIndex)
{
	std::string ignore;
    boneIndexToParentIndex.resize(numBones);

    fin >> ignore; // BoneHierarchy header text
	for(uint32_t i = 0; i < numBones; ++i)
	{
	    fin >> ignore >> boneIndexToParentIndex[i];
	}
}

void M3dLoader::ReadAnimationClips(std::ifstream& fin, uint32_t numBones, uint32_t numAnimationClips,
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

void M3dLoader::ReadBoneKeyframes(std::ifstream& fin, uint32_t numBones, BoneAnimation& boneAnimation)
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