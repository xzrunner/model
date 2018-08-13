#include "model/ModelInstance.h"
#include "model/Model.h"
#include "model/GlobalClock.h"
#include "model/MorphTargetAnim.h"
#include "model/SkeletalAnim.h"

namespace model
{

ModelInstance::ModelInstance(const std::shared_ptr<Model>& m_model, int anim_idx)
	: m_model(m_model)
	, m_curr_anim_index(anim_idx)
{
	auto& ext = m_model->ext;
	if (ext && ext->Type() == EXT_SKELETAL)
	{
		auto sk_anim = static_cast<SkeletalAnim*>(ext.get());
		auto& nodes = sk_anim->GetAllNodes();
		int sz = nodes.size();

		// local trans
		m_local_trans.reserve(sz);
		for (int i = 0; i < sz; ++i) {
			m_local_trans.push_back(nodes[i]->local_trans);
		}

		// global trans
		CalcGlobalTrans();

		auto& anims = sk_anim->GetAllAnims();
		if (m_curr_anim_index >= 0 && m_curr_anim_index < static_cast<int>(anims.size()))
		{
			m_channel_idx.reserve(sz);
			auto& channels = anims[m_curr_anim_index]->channels;
			for (int i = 0; i < sz; ++i)
			{
				int idx = -1;
				for (int j = 0, m = channels.size(); j < m; ++j) {
					if (nodes[i]->name == channels[j]->name) {
						idx = j;
					}
				}
				m_channel_idx.push_back(idx);
			}

			m_last_pos.assign(channels.size(), std::make_tuple(0, 0, 0));
		}
	}
}

bool ModelInstance::Update()
{
	if (!m_model->ext) {
		return false;
	}

	switch (m_model->ext->Type())
	{
	case EXT_MORPH_TARGET:
		return UpdateMorphTargetAnim();
	case EXT_SKELETAL:
		return UpdateSkeletalAnim();
	}
	return false;
}

bool ModelInstance::SetFrame(int curr_frame)
{
	if (!m_model->ext) {
		return false;
	}
	if (m_model->ext->Type() != EXT_SKELETAL) {
		return false;
	}

	auto sk_anim = static_cast<SkeletalAnim*>(m_model->ext.get());

	auto& anims = sk_anim->GetAllAnims();
	auto& ext = anims[m_curr_anim_index];

	std::vector<sm::mat4> channels_trans(ext->channels.size());

	// calc anim trans
	for (int i = 0, n = ext->channels.size(); i < n; ++i)
	{
		auto& channel = ext->channels[i];

		// position
		sm::vec3 position(0, 0, 0);
		if (!channel->position_keys.empty())
		{
			unsigned int frame = 0;
			while (frame < channel->position_keys.size() - 1)
			{
				if (curr_frame < channel->position_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->position_keys.size();
			auto& key = channel->position_keys[frame];
			auto& next_key = channel->position_keys[next_frame];
			float diff_time = next_key.first- key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				position = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				position = key.second;
			}

			std::get<0>(m_last_pos[i]) = frame;
		}

		// rotation
		sm::Quaternion rotation(1, 0, 0, 0);
		if (!channel->rotation_keys.empty())
		{
			unsigned int frame = 0;
			while (frame < channel->rotation_keys.size() - 1)
			{
				if (curr_frame < channel->rotation_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->rotation_keys.size();
			auto& key = channel->rotation_keys[frame];
			auto& next_key = channel->rotation_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				rotation.Slerp(key.second, next_key.second, factor);
			}
			else
			{
				rotation = key.second;
			}

			std::get<1>(m_last_pos[i]) = frame;
		}

		// scaling
		sm::vec3 scaling(0, 0, 0);
		if (!channel->scaling_keys.empty())
		{
			unsigned int frame = 0;
			while (frame < channel->scaling_keys.size() - 1)
			{
				if (curr_frame < channel->scaling_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->scaling_keys.size();
			auto& key = channel->scaling_keys[frame];
			auto& next_key = channel->scaling_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				scaling = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				scaling = key.second;
			}

			std::get<2>(m_last_pos[i]) = frame;
		}

		sm::mat4 m(rotation);
		m.c[0][0] *= scaling.x; m.c[0][1] *= scaling.x; m.c[0][2] *= scaling.x; m.c[0][3] = 0;
		m.c[1][0] *= scaling.y; m.c[1][1] *= scaling.y; m.c[1][2] *= scaling.y; m.c[1][3] = 0;
		m.c[2][0] *= scaling.z; m.c[2][1] *= scaling.z; m.c[2][2] *= scaling.z; m.c[2][3] = 0;
		m.c[3][0] = position.x; m.c[3][1] = position.y; m.c[3][2] = position.z; m.c[3][3] = 1;
		channels_trans[i] = m;
	}

	// update local trans
	assert(m_channel_idx.size() == m_local_trans.size());
	for (int i = 0, n = m_channel_idx.size(); i < n; ++i)
	{
		if (m_channel_idx[i] >= 0) {
			m_local_trans[i] = channels_trans[m_channel_idx[i]];
		} else {
			m_local_trans[i].Identity();
		}
	}

	// update global trans
	CalcGlobalTrans();

	return true;
}

void ModelInstance::RotateJoint(int idx, const sm::Quaternion& delta)
{
	assert(idx >= 0 && idx < m_local_trans.size());
	m_local_trans[idx] = sm::mat4(delta) * m_local_trans[idx];
	CalcGlobalTrans();
}

bool ModelInstance::UpdateMorphTargetAnim()
{
	float curr_time = GlobalClock::Instance()->GetTime() * m_model->anim_speed;
	if (m_start_time == 0)
	{
		m_start_time = curr_time;
		m_last_time = 0;
		return false;
	}

	auto ext = static_cast<MorphTargetAnim*>(m_model->ext.get());
	if (ext->GetNumFrames() == 1)
	{
		ext->SetFrame(0);
		ext->SetBlend(0);
	}
	else
	{
		float f_frame = (curr_time - m_start_time) * ext->GetFps();
		int frame = static_cast<int>(f_frame) % ext->GetNumFrames();
		ext->SetFrame(frame);
		float blend = f_frame - std::floor(f_frame);
		ext->SetBlend(blend);
	}

	return true;
}

bool ModelInstance::UpdateSkeletalAnim()
{
	auto sk_anim = static_cast<SkeletalAnim*>(m_model->ext.get());

	auto& anims = sk_anim->GetAllAnims();
	if (m_curr_anim_index < 0 || m_curr_anim_index >= static_cast<int>(anims.size())) {
		return false;
	}

	float curr_time = GlobalClock::Instance()->GetTime() * m_model->anim_speed;
	if (m_start_time == 0)
	{
		m_start_time = curr_time;
		m_last_time = 0;
		return false;
	}

	auto& ext = anims[m_curr_anim_index];

	float curr_frame = 0.0f;
	float ticks_per_second = ext->ticks_per_second != 0 ? ext->ticks_per_second : 25.0f;
	if (ext->duration > 0) {
		curr_time = fmod(curr_time - m_start_time, ext->duration / ticks_per_second);
		curr_frame = curr_time * ticks_per_second;
	}

	std::vector<sm::mat4> channels_trans(ext->channels.size());

	// calc anim trans
	for (int i = 0, n = ext->channels.size(); i < n; ++i)
	{
		auto& channel = ext->channels[i];

		// position
		sm::vec3 position(0, 0, 0);
		if (!channel->position_keys.empty())
		{
			unsigned int frame = (curr_time >= m_last_time) ? std::get<0>(m_last_pos[i]) : 0;
			while (frame < channel->position_keys.size() - 1)
			{
				if (curr_frame < channel->position_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->position_keys.size();
			auto& key = channel->position_keys[frame];
			auto& next_key = channel->position_keys[next_frame];
			float diff_time = next_key.first- key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				position = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				position = key.second;
			}

			std::get<0>(m_last_pos[i]) = frame;
		}

		// rotation
		sm::Quaternion rotation(1, 0, 0, 0);
		if (!channel->rotation_keys.empty())
		{
			unsigned int frame = (curr_time >= m_last_time) ? std::get<1>(m_last_pos[i]) : 0;
			while (frame < channel->rotation_keys.size() - 1)
			{
				if (curr_frame < channel->rotation_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->rotation_keys.size();
			auto& key = channel->rotation_keys[frame];
			auto& next_key = channel->rotation_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				rotation.Slerp(key.second, next_key.second, factor);
			}
			else
			{
				rotation = key.second;
			}

			std::get<1>(m_last_pos[i]) = frame;
		}

		// scaling
		sm::vec3 scaling(0, 0, 0);
		if (!channel->scaling_keys.empty())
		{
			unsigned int frame = (curr_time >= m_last_time) ? std::get<2>(m_last_pos[i]) : 0;
			while (frame < channel->scaling_keys.size() - 1)
			{
				if (curr_frame < channel->scaling_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->scaling_keys.size();
			auto& key = channel->scaling_keys[frame];
			auto& next_key = channel->scaling_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += ext->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((curr_frame - key.first) / diff_time);
				scaling = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				scaling = key.second;
			}

			std::get<2>(m_last_pos[i]) = frame;
		}

		sm::mat4 m(rotation);
		m.c[0][0] *= scaling.x; m.c[0][1] *= scaling.x; m.c[0][2] *= scaling.x; m.c[0][3] = 0;
		m.c[1][0] *= scaling.y; m.c[1][1] *= scaling.y; m.c[1][2] *= scaling.y; m.c[1][3] = 0;
		m.c[2][0] *= scaling.z; m.c[2][1] *= scaling.z; m.c[2][2] *= scaling.z; m.c[2][3] = 0;
		m.c[3][0] = position.x; m.c[3][1] = position.y; m.c[3][2] = position.z; m.c[3][3] = 1;
		channels_trans[i] = m;
	}

	// update local trans
	assert(m_channel_idx.size() == m_local_trans.size());
	for (int i = 0, n = m_channel_idx.size(); i < n; ++i)
	{
		if (m_channel_idx[i] >= 0) {
			m_local_trans[i] = channels_trans[m_channel_idx[i]];
		} else {
			m_local_trans[i].Identity();
		}
	}

	// update global trans
	CalcGlobalTrans();

	m_last_time = curr_time;

	return true;
}

const std::vector<sm::mat4>& ModelInstance::CalcBoneMatrices(int node_idx, int mesh_idx) const
{
	if (!m_model->ext || m_model->ext->Type() != EXT_SKELETAL) {
		return m_bone_trans;
	}

	auto& mesh = m_model->meshes[mesh_idx];

	sm::mat4 global_inv_mesh_trans = m_global_trans[node_idx].Inverted();

	m_bone_trans.resize(mesh->geometry.bones.size());
	for (size_t i = 0, n = mesh->geometry.bones.size(); i < n; ++i)
	{
		auto& bone = mesh->geometry.bones[i];
		assert(bone.node >= 0);
		m_bone_trans[i] = bone.offset_trans * m_global_trans[bone.node] * global_inv_mesh_trans;
	}

	return m_bone_trans;
}

void ModelInstance::CalcGlobalTrans()
{
	if (!m_model->ext || m_model->ext->Type() != EXT_SKELETAL) {
		return;
	}

	auto sk_anim = static_cast<SkeletalAnim*>(m_model->ext.get());
	auto& nodes = sk_anim->GetAllNodes();
	size_t sz = m_local_trans.size();
	if (m_global_trans.size() != sz) {
		m_global_trans.resize(sz);
	}
	for (size_t i = 0; i < sz; ++i)
	{
		auto g_trans = m_local_trans[i];
		int parent = nodes[i]->parent;
		while (parent != -1) {
			g_trans = g_trans * m_local_trans[parent];
			parent = nodes[parent]->parent;
		}
		m_global_trans[i] = g_trans;
	}
}

}