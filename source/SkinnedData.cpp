// This code from d3d12book https://github.com/d3dcoder/d3d12book

#include "model/SkinnedData.h"

#include <algorithm>

namespace model
{

Keyframe::Keyframe()
	: TimePos(0.0f)
	, Translation(0.0f, 0.0f, 0.0f)
	, Scale(1.0f, 1.0f, 1.0f)
	, RotationQuat(0.0f, 0.0f, 0.0f, 1.0f)
{
}

Keyframe::~Keyframe()
{
}

float BoneAnimation::GetStartTime()const
{
	// Keyframes are sorted by time, so first keyframe gives start time.
	return Keyframes.front().TimePos;
}

float BoneAnimation::GetEndTime()const
{
	// Keyframes are sorted by time, so last keyframe gives end time.
	float f = Keyframes.back().TimePos;

	return f;
}

void BoneAnimation::Interpolate(float t, sm::mat4& M)const
{
	if( t <= Keyframes.front().TimePos )
	{
		auto& frame = Keyframes.front();
		sm::vec3 rotation_origin(0, 0, 0);
		M.SetTransformation(frame.Scale, rotation_origin, frame.RotationQuat, frame.Translation);
	}
	else if( t >= Keyframes.back().TimePos )
	{
		auto& frame = Keyframes.back();
		sm::vec3 rotation_origin(0, 0, 0);
		M.SetTransformation(frame.Scale, rotation_origin, frame.RotationQuat, frame.Translation);
	}
	else
	{
		for(uint32_t i = 0; i < Keyframes.size()-1; ++i)
		{
			if( t >= Keyframes[i].TimePos && t <= Keyframes[i+1].TimePos )
			{
				float lerpPercent = (t - Keyframes[i].TimePos) / (Keyframes[i+1].TimePos - Keyframes[i].TimePos);

				auto& s0 = Keyframes[i].Scale;
				auto& s1 = Keyframes[i+1].Scale;

				auto& p0 = Keyframes[i].Translation;
				auto& p1 = Keyframes[i+1].Translation;

				auto& q0 = Keyframes[i].RotationQuat;
				auto& q1 = Keyframes[i+1].RotationQuat;

				auto S = s0 + (s1 - s0) * lerpPercent;
				auto P = p0 + (p1 - s0) * lerpPercent;
				sm::Quaternion Q;
				Q.Slerp(sm::Quaternion(q0), sm::Quaternion(q1), lerpPercent);

				sm::vec3 rotation_origin(0, 0, 0);
				M.SetTransformation(S, rotation_origin, Q.ToVector(), P);

				break;
			}
		}
	}
}

float AnimationClip::GetClipStartTime()const
{
	// Find smallest start time over all bones in this clip.
	float t = FLT_MAX;
	for(uint32_t i = 0; i < BoneAnimations.size(); ++i) {
		t = std::min(t, BoneAnimations[i].GetStartTime());
	}

	return t;
}

float AnimationClip::GetClipEndTime()const
{
	// Find largest end time over all bones in this clip.
	float t = 0.0f;
	for(uint32_t i = 0; i < BoneAnimations.size(); ++i) {
		t = std::max(t, BoneAnimations[i].GetEndTime());
	}

	return t;
}

void AnimationClip::Interpolate(float t, std::vector<sm::mat4>& boneTransforms)const
{
	for(uint32_t i = 0; i < BoneAnimations.size(); ++i)
	{
		BoneAnimations[i].Interpolate(t, boneTransforms[i]);
	}
}

float SkinnedData::GetClipStartTime(const std::string& clipName)const
{
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipStartTime();
}

float SkinnedData::GetClipEndTime(const std::string& clipName)const
{
	auto clip = mAnimations.find(clipName);
	return clip->second.GetClipEndTime();
}

size_t SkinnedData::BoneCount()const
{
	return mBoneHierarchy.size();
}

void SkinnedData::Set(std::vector<int>& boneHierarchy,
		              std::vector<sm::mat4>& boneOffsets,
		              std::unordered_map<std::string, AnimationClip>& animations)
{
	mBoneHierarchy = boneHierarchy;
	mBoneOffsets   = boneOffsets;
	mAnimations    = animations;
}

void SkinnedData::GetFinalTransforms(const std::string& clipName, float timePos,  std::vector<sm::mat4>& finalTransforms)const
{
	uint32_t numBones = mBoneOffsets.size();

	std::vector<sm::mat4> toParentTransforms(numBones);

	// Interpolate all the bones of this clip at the given time instance.
	auto clip = mAnimations.find(clipName);
	clip->second.Interpolate(timePos, toParentTransforms);

	//
	// Traverse the hierarchy and transform all the bones to the root space.
	//

	std::vector<sm::mat4> toRootTransforms(numBones);

	// The root bone has index 0.  The root bone has no parent, so its toRootTransform
	// is just its local bone transform.
	toRootTransforms[0] = toParentTransforms[0];

	// Now find the toRootTransform of the children.
	for(uint32_t i = 1; i < numBones; ++i)
	{
		auto& toParent = toParentTransforms[i];

		int parentIndex = mBoneHierarchy[i];
		auto& parentToRoot = toRootTransforms[parentIndex];

		toRootTransforms[i] = parentToRoot * toParent;  // mat mul
	}

	// Premultiply by the bone offset transform to get the final transform.
	for(uint32_t i = 0; i < numBones; ++i)
	{
		auto& offset = mBoneOffsets[i];
		auto& toRoot = toRootTransforms[i];
		finalTransforms[i] = toRoot * offset;   // mat mul
	}
}

}