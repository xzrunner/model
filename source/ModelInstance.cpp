#include "model/ModelInstance.h"
#include "model/Model.h"
#include "model/GlobalClock.h"

namespace model
{

ModelInstance::ModelInstance(const std::shared_ptr<Model>& model, int anim_idx)
	: model(model)
	, curr_anim_index(anim_idx)
{
	int sz = model->nodes.size();

	// local trans
	local_trans.reserve(sz);
	for (int i = 0; i < sz; ++i) {
		local_trans.push_back(model->nodes[i]->local_trans);
	}

	// global trans
	CalcGlobalTrans();

	if (curr_anim_index >= 0 && curr_anim_index < model->anims.size())
	{
		channel_idx.reserve(sz);
		auto& channels = model->anims[curr_anim_index]->channels;
		for (int i = 0; i < sz; ++i)
		{
			int idx = -1;
			for (int j = 0, m = channels.size(); j < m; ++j) {
				if (model->nodes[i]->name == channels[j]->name) {
					idx = j;
				}
			}
			channel_idx.push_back(idx);
		}

		last_pos.assign(channels.size(), std::make_tuple(0, 0, 0));
	}
}

bool ModelInstance::Update()
{
	float curr_time = GlobalClock::Instance()->GetTime();
	if (last_time >= curr_time || last_time == 0) {
		last_time = curr_time;
		return false;
	}

	float dt = curr_time - last_time;
	last_time = curr_time;

	if (curr_anim_index < 0 || curr_anim_index >= model->anims.size()) {
		return false;
	}

	auto& anim = model->anims[curr_anim_index];

	float ticks_per_second = anim->ticks_per_second != 0.0 ? anim->ticks_per_second : 25.0;
	dt *= ticks_per_second;

	float time = 0.0f;
	if (anim->duration > 0.0) {
		time = fmod(dt, anim->duration);
	}

	if (anim_trans.size() != anim->channels.size()) {
		anim_trans.resize(anim->channels.size());
	}

	// calc anim trans
	for (int i = 0, n = anim->channels.size(); i < n; ++i)
	{
		auto& channel = anim->channels[i];

		// position
		sm::vec3 position(0, 0, 0);
		if (!channel->position_keys.empty())
		{
			unsigned int frame = (time >= last_time) ? std::get<0>(last_pos[i]) : 0;
			while (frame < channel->position_keys.size() - 1)
			{
				if (time < channel->position_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->position_keys.size();
			auto& key = channel->position_keys[frame];
			auto& next_key = channel->position_keys[next_frame];
			float diff_time = next_key.first- key.first;
			if (diff_time < 0.0) {
				diff_time += anim->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((time - key.first) / diff_time);
				position = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				position = key.second;
			}

			std::get<0>(last_pos[i]) = frame;
		}

		// rotation
		sm::Quaternion rotation(1, 0, 0, 0);
		if (!channel->rotation_keys.empty())
		{
			unsigned int frame = (time >= last_time) ? std::get<1>(last_pos[i]) : 0;
			while (frame < channel->rotation_keys.size() - 1)
			{
				if (time < channel->rotation_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->rotation_keys.size();
			auto& key = channel->rotation_keys[frame];
			auto& next_key = channel->rotation_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += anim->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((time - key.first) / diff_time);
				rotation.Slerp(key.second, next_key.second, factor);
			}
			else
			{
				rotation = key.second;
			}

			std::get<1>(last_pos[i]) = frame;
		}

		// scaling
		sm::vec3 scaling(0, 0, 0);
		if (!channel->scaling_keys.empty())
		{
			unsigned int frame = (time >= last_time) ? std::get<2>(last_pos[i]) : 0;
			while (frame < channel->scaling_keys.size() - 1)
			{
				if (time < channel->scaling_keys[frame + 1].first) {
					break;
				}
				frame++;
			}

			unsigned int next_frame = (frame + 1) % channel->scaling_keys.size();
			auto& key = channel->scaling_keys[frame];
			auto& next_key = channel->scaling_keys[next_frame];
			float diff_time = next_key.first - key.first;
			if (diff_time < 0.0) {
				diff_time += anim->duration;
			}
			if (diff_time > 0)
			{
				float factor = float((time - key.first) / diff_time);
				scaling = key.second + (next_key.second - key.second) * factor;
			}
			else
			{
				scaling = key.second;
			}

			std::get<2>(last_pos[i]) = frame;
		}

		sm::mat4 m(rotation);
		m.c[0][0] *= scaling.x; m.c[0][1] *= scaling.x; m.c[0][2] *= scaling.x; m.c[0][3] = 0;
		m.c[1][0] *= scaling.y; m.c[1][1] *= scaling.y; m.c[1][2] *= scaling.y; m.c[1][3] = 0;
		m.c[2][0] *= scaling.z; m.c[2][1] *= scaling.z; m.c[2][2] *= scaling.z; m.c[2][3] = 0;
		m.c[3][0] = position.x; m.c[3][1] = position.y; m.c[3][2] = position.z; m.c[3][3] = 1;
		anim_trans[i] = m;
	}

	// update local trans
	assert(channel_idx.size() == local_trans.size());
	for (int i = 0, n = channel_idx.size(); i < n; ++i) {
		if (channel_idx[i] >= 0) {
			local_trans[i] = anim_trans[channel_idx[i]];
		}
	}

	// update global trans
	CalcGlobalTrans();

	last_time = time;
}

void ModelInstance::CalcGlobalTrans()
{
	size_t sz = local_trans.size();
	if (global_trans.size() != sz) {
		global_trans.resize(sz);
	}
	for (size_t i = 0; i < sz; ++i)
	{
		auto g_trans = local_trans[i];
		int parent = model->nodes[i]->parent;
		while (parent != -1) {
			g_trans = g_trans * local_trans[parent];
			parent = model->nodes[parent]->parent;
		}
		global_trans[i] = g_trans;
	}
}

}