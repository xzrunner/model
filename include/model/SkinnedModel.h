// This code from d3d12book https://github.com/d3dcoder/d3d12book

#pragma once

#include "model/SkinnedData.h"

#include <SM_Matrix.h>
#include <unirender/noncopyable.h>

#include <vector>

namespace model
{

class SkinnedData;
struct SkinnedModelInstance : ur::noncopyable
{
	SkinnedData* SkinnedInfo = nullptr;
	std::vector<sm::mat4> FinalTransforms;
	std::string ClipName;
	float TimePos = 0.0f;

	void UpdateSkinnedAnimation(float dt)
	{
		TimePos += dt;

		if (TimePos > SkinnedInfo->GetClipEndTime(ClipName))
			TimePos = 0.0f;

		SkinnedInfo->GetFinalTransforms(ClipName, TimePos, FinalTransforms);
	}
};

struct RenderItem : ur::noncopyable
{
	struct MeshMaterial
	{
		sm::vec4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
		sm::vec3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
		float Roughness = .25f;
	};

	struct MeshGeometry
	{
		unsigned int vao = 0;
	};

	MeshMaterial* Mat = nullptr;
	MeshGeometry Geo;

	SkinnedModelInstance* SkinnedModelInst = nullptr;
};

}