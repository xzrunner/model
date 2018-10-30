#include "model/AnimIK.h"
#include "model/ModelInstance.h"
#include "model/SkeletalAnim.h"
#include "model/Model.h"

#include <SM_Matrix.h>
#include <SM_Calc.h>

namespace
{

sm::mat4 get_rot_inv(const sm::mat4& mat)
{
	auto inv = mat;
	inv.x[12] = inv.x[13] = inv.x[14] = 0;
	return inv.Inverted();
}

}

namespace model
{

void AnimIK::OneBone(ModelInstance& model, int joint, const sm::vec3& target, 
	                 std::array<sm::vec3, 3>& debug_pos)
{
	auto sk_anim = static_cast<SkeletalAnim*>(model.GetModel()->ext.get());
	auto& bones = sk_anim->GetNodes();
	int p = bones[joint]->parent;
	if (p < 0) {
		return;
	}
	int pp = bones[p]->parent;
	if (pp < 0) {
		return;
	}

	auto& g_trans = model.GetGlobalTrans();
	auto p_pos = g_trans[p] * sm::vec3(0, 0, 0);

	auto pp_inv = get_rot_inv(g_trans[pp]);

	auto& tp_wtrans = sk_anim->GetTPWorldTrans();
	auto u = (tp_wtrans[joint] * sm::vec3(0, 0, 0) - tp_wtrans[p] * sm::vec3(0, 0, 0)).Normalized();
	auto v = (target - p_pos).Normalized();

	debug_pos[0] = p_pos;
	debug_pos[1] = p_pos + u;
	debug_pos[2] = p_pos + v;

	v = pp_inv * v;

	model.SetJointRotate(p, sm::Quaternion::CreateFromVectors(u, v));
}

void AnimIK::TwoBones(ModelInstance& model, int joint, const sm::vec3& target, 
	                  const sm::vec3& rot_axis, std::array<sm::vec3, 3>& debug_pos)
{
	auto sk_anim = static_cast<SkeletalAnim*>(model.GetModel()->ext.get());
	auto& bones = sk_anim->GetNodes();
	int p = bones[joint]->parent;
	if (p < 0) {
		return;
	}
	int pp = bones[p]->parent;
	if (pp < 0) {
		return;
	}
	int ppp = bones[pp]->parent;
	if (ppp < 0) {
		return;
	}

	auto& g_trans = model.GetGlobalTrans();
	auto c_pos  = g_trans[joint] * sm::vec3(0, 0, 0);
	auto p_pos  = g_trans[p]     * sm::vec3(0, 0, 0);
	auto pp_pos = g_trans[pp]    * sm::vec3(0, 0, 0);

	auto& tp_wtrans = sk_anim->GetTPWorldTrans();

	float len0 = sm::dis_pos3_to_pos3(pp_pos, p_pos);
	float len1 = sm::dis_pos3_to_pos3(p_pos, c_pos);
	float tot_len = sm::dis_pos3_to_pos3(target, pp_pos);
	if (tot_len < len0 + len1 && len0 < tot_len + len1 && len1 < tot_len + len0)
	{
		float ang0 = acosf((len0 * len0 + tot_len * tot_len - len1 * len1) / (2 * len0 * tot_len));
		float ang1 = acosf((len1 * len1 + tot_len * tot_len - len0 * len0) / (2 * len1 * tot_len));
		//model.SetJointRotate(pp, bones[pp]->local_trans, sm::Quaternion::CreateFromEulerAngle(-(angle - ang0), 0, 0));
		//model.SetJointRotate(p, bones[p]->local_trans, sm::Quaternion::CreateFromEulerAngle(-(ang0 + ang1), 0, 0));

		printf("ang0 %f, ang1 %f, len0 %f, len1 %f\n", ang0, ang1, len0, len1);

//		auto rot_mat = sm::Quaternion::CreateFromAxisAngle(rot_axis, ang0);
		auto rot_mat = sm::Quaternion::CreateFromAxisAngle({1, 0, 0}, ang0);
		auto new_pos = sm::mat4(rot_mat) * ((target - pp_pos).Normalized() * len0) + pp_pos;

		debug_pos[0] = pp_pos;
		debug_pos[1] = new_pos;
		debug_pos[2] = target;

		{
			sm::mat4 ppp_inv = get_rot_inv(g_trans[ppp]);

			auto u = (tp_wtrans[p] * sm::vec3(0, 0, 0) - tp_wtrans[pp] * sm::vec3(0, 0, 0)).Normalized();
			auto v = (new_pos - pp_pos).Normalized();
			v = ppp_inv * v;
			model.SetJointRotate(pp, sm::Quaternion::CreateFromVectors(u, v));
		}
		{
			sm::mat4 pp_inv = get_rot_inv(g_trans[pp]);

			auto u = (tp_wtrans[joint] * sm::vec3(0, 0, 0) - tp_wtrans[p] * sm::vec3(0, 0, 0)).Normalized();
			auto v = (target - new_pos).Normalized();
			v = pp_inv * v;
			model.SetJointRotate(p, sm::Quaternion::CreateFromVectors(u, v));
		}
	}
	else
	{
		sm::mat4 ppp_inv = get_rot_inv(g_trans[ppp]);

		auto u = (tp_wtrans[joint] * sm::vec3(0, 0, 0) - tp_wtrans[pp] * sm::vec3(0, 0, 0)).Normalized();
		auto v = (target - pp_pos).Normalized();

		debug_pos[0] = pp_pos;
		debug_pos[1] = pp_pos + u;
		debug_pos[2] = target;

		v = ppp_inv * v;

		model.SetJointRotate(pp, sm::Quaternion::CreateFromVectors(u, v));
		model.SetJointRotate(p, sm::Quaternion());
	}
}

}