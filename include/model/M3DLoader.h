// This code from d3d12book https://github.com/d3dcoder/d3d12book

#pragma once

#include <SM_Vector.h>
#include <SM_Matrix.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace model
{

class Model;
class SkinnedData;
struct BoneAnimation;
struct AnimationClip;

class M3DLoader
{
public:
	static bool Load(Model& model, const std::string& filepath);

private:
	struct Vertex
	{
		sm::vec3 Pos;
		sm::vec3 Normal;
		sm::vec2 TexC;
		sm::vec4 TangentU;
	};

	struct SkinnedVertex
	{
		sm::vec3 Pos;
		sm::vec3 Normal;
		sm::vec2 TexC;
		sm::vec3 TangentU;
		sm::vec3 BoneWeights;
		uint32_t  BoneIndices[4];
	};

	struct Subset
	{
		uint32_t Id = -1;
		uint32_t VertexStart = 0;
		uint32_t VertexCount = 0;
		uint32_t FaceStart = 0;
		uint32_t FaceCount = 0;
	};

	struct M3dMaterial
	{
		std::string Name;

		sm::vec4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		sm::vec3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = 0.8f;
		bool AlphaClip = false;

		std::string MaterialTypeName;
		std::string DiffuseMapName;
		std::string NormalMapName;
	};

	static bool Load(const std::string& filename,
		             std::vector<Vertex>& vertices,
		             std::vector<uint16_t>& indices,
		             std::vector<Subset>& subsets,
		             std::vector<M3dMaterial>& mats);
	static bool Load(const std::string& filename,
		             std::vector<SkinnedVertex>& vertices,
		             std::vector<uint16_t>& indices,
		             std::vector<Subset>& subsets,
		             std::vector<M3dMaterial>& mats,
		             SkinnedData& skinInfo);

private:
	static void ReadMaterials(std::ifstream& fin, uint32_t numMaterials, std::vector<M3dMaterial>& mats);
	static void ReadSubsetTable(std::ifstream& fin, uint32_t numSubsets, std::vector<Subset>& subsets);
	static void ReadVertices(std::ifstream& fin, uint32_t numVertices, std::vector<Vertex>& vertices);
	static void ReadSkinnedVertices(std::ifstream& fin, uint32_t numVertices, std::vector<SkinnedVertex>& vertices);
	static void ReadTriangles(std::ifstream& fin, uint32_t numTriangles, std::vector<uint16_t>& indices);
	static void ReadBoneOffsets(std::ifstream& fin, uint32_t numBones, std::vector<sm::mat4>& boneOffsets);
	static void ReadBoneHierarchy(std::ifstream& fin, uint32_t numBones, std::vector<int>& boneIndexToParentIndex);
	static void ReadAnimationClips(std::ifstream& fin, uint32_t numBones, uint32_t numAnimationClips, std::unordered_map<std::string, AnimationClip>& animations);
	static void ReadBoneKeyframes(std::ifstream& fin, uint32_t numBones, BoneAnimation& boneAnimation);

}; // M3DLoader

}