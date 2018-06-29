#pragma once

#include <SM_Matrix.h>

#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>
#include <tuple>

namespace model
{

struct Model;

struct ModelInstance : boost::noncopyable
{
	ModelInstance(const std::shared_ptr<Model>& model, int anim_idx = 0);

	bool Update();

	const std::vector<sm::mat4>& CalcBoneMatrices(int node, int mesh) const;

	std::shared_ptr<Model> model = nullptr;

	int curr_anim_index = -1;

	std::vector<sm::mat4> local_trans;
	std::vector<sm::mat4> global_trans;

	std::vector<int> channel_idx;

private:
	bool UpdateMorphTargetAnim();
	bool UpdateSkeletalAnim();

	void CalcGlobalTrans();

private:
	float m_last_time = 0;
	float m_start_time = 0;

	std::vector<std::tuple<unsigned int, unsigned int, unsigned int> > m_last_pos;

	mutable std::vector<sm::mat4> m_bone_trans;

}; // ModelInstance

}