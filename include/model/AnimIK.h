#pragma once

#include <SM_Vector.h>

#include <array>

namespace model
{

class ModelInstance;

class AnimIK
{
public:
	static void OneBone(ModelInstance& model, int joint, 
		const sm::vec3& target, std::array<sm::vec3, 3>& debug_pos);

	static void TwoBones(ModelInstance& model, int joint, const sm::vec3& rot_axis,
		const sm::vec3& target, std::array<sm::vec3, 3>& debug_pos);

}; // AnimIK

}