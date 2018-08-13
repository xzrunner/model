#pragma once

#include <SM_Matrix.h>

#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>
#include <tuple>

namespace model
{

struct Model;

class ModelInstance : boost::noncopyable
{
public:
	ModelInstance(const std::shared_ptr<Model>& model, int anim_idx = 0);

	bool Update();
	bool SetFrame(int frame);

	const std::vector<sm::mat4>& CalcBoneMatrices(int node, int mesh) const;

	const std::shared_ptr<Model>& GetModel() const { return m_model; }

	int  GetCurrAnimIndex() const { return m_curr_anim_index; }
	void SetCurrAnimIndex(int idx) { m_curr_anim_index = idx; }

	auto& GetLocalTrans() const { return m_local_trans; }
	auto& GetGlobalTrans() const { return m_global_trans; }

	void RotateJoint(int idx, const sm::Quaternion& delta);
	void TranslateJoint(int idx, const sm::vec3& offset);

private:
	bool UpdateMorphTargetAnim();
	bool UpdateSkeletalAnim();

	void CalcGlobalTrans();

private:
	std::shared_ptr<Model> m_model = nullptr;

	int m_curr_anim_index = -1;

	std::vector<sm::mat4> m_local_trans;
	std::vector<sm::mat4> m_global_trans;

	std::vector<int> m_channel_idx;

	float m_last_time = 0;
	float m_start_time = 0;

	std::vector<std::tuple<unsigned int, unsigned int, unsigned int> > m_last_pos;

	mutable std::vector<sm::mat4> m_bone_trans;

}; // ModelInstance

}