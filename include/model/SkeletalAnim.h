#pragma once

#include "model/ModelExtend.h"

#include <SM_Matrix.h>

#include <string>
#include <vector>
#include <memory>

#include <math.h>

namespace model
{

class SkeletalAnim : public ModelExtend
{
public:
	struct Node : boost::noncopyable
	{
		std::string name;

		int parent = -1;
		std::vector<int> children;

		std::vector<int> meshes;

		sm::mat4 local_trans;

		int channel_idx = -1;

	}; // Node

	struct NodeAnim
	{
		std::string name;

		std::vector<std::pair<float, sm::vec3>>       position_keys;
		std::vector<std::pair<float, sm::Quaternion>> rotation_keys;
		std::vector<std::pair<float, sm::vec3>>       scaling_keys;

	}; // NodeAnim

	struct ModelExtend
	{
		std::string name;

		float duration = 0;

		float ticks_per_second = 0;

		std::vector<std::unique_ptr<NodeAnim>> channels;

		int GetMaxFrameCount() const {
			return static_cast<int>(roundf(duration * ticks_per_second)) + 1;
		}

	}; // ModelExtend

public:
	virtual ModelExtendType Type() const override { return EXT_SKELETAL; }

	void  SetAnims(std::vector<std::unique_ptr<ModelExtend>>& anims);
	auto& GetAnims() const { return m_anims; }

	void  SetNodes(std::vector<std::unique_ptr<Node>>& nodes);
	auto& GetNodes() const { return m_nodes; }

	auto& GetTPWorldTrans() const { return m_tpose_world_trans; }

	void PrintNodeTree() const;

private:
	void InitTPoseTrans();

private:
	std::vector<std::unique_ptr<Node>> m_nodes;

	std::vector<sm::mat4> m_tpose_world_trans;
	
	std::vector<std::unique_ptr<ModelExtend>> m_anims;

}; // SkeletalAnim

}