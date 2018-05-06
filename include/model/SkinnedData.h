#pragma once

#include <SM_Vector.h>
#include <SM_Matrix.h>

#include <vector>
#include <unordered_map>

namespace model
{

struct Keyframe
{
	Keyframe();
	~Keyframe();

    float TimePos;
	sm::vec3 Translation;
    sm::vec3 Scale;
    sm::vec4 RotationQuat;
};

struct BoneAnimation
{
	float GetStartTime()const;
	float GetEndTime()const;

    void Interpolate(float t, sm::mat4& M)const;

	std::vector<Keyframe> Keyframes;

};

struct AnimationClip
{
	float GetClipStartTime()const;
	float GetClipEndTime()const;

    void Interpolate(float t, std::vector<sm::mat4>& boneTransforms)const;

    std::vector<BoneAnimation> BoneAnimations;

};

class SkinnedData
{
public:

	size_t BoneCount()const;

	float GetClipStartTime(const std::string& clipName)const;
	float GetClipEndTime(const std::string& clipName)const;

	void Set(
		std::vector<int>& boneHierarchy,
		std::vector<sm::mat4>& boneOffsets,
		std::unordered_map<std::string, AnimationClip>& animations);

    void GetFinalTransforms(const std::string& clipName, float timePos,
		 std::vector<sm::mat4>& finalTransforms)const;

private:
	std::vector<int> mBoneHierarchy;

	std::vector<sm::mat4> mBoneOffsets;

	std::unordered_map<std::string, AnimationClip> mAnimations;
};

}