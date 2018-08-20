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
			return static_cast<int>(roundf(duration * ticks_per_second));
		}

	}; // ModelExtend

public:
	virtual ModelExtendType Type() const override { return EXT_SKELETAL; }

	int QueryNodeByName(const std::string& name) const;

	void AddAnim(std::unique_ptr<ModelExtend>& ext) {
		m_anims.push_back(std::move(ext));
	}
	auto& GetAllAnims() const { return m_anims; }

	int GetNodeSize() const { return m_nodes.size(); }

	void AddNode(std::unique_ptr<Node>& node) {
		m_nodes.push_back(std::move(node));
	}
	Node* GetNode(int idx) {
		if (idx < 0 || idx >= static_cast<int>(m_nodes.size())) {
			return nullptr;
		} else {
			return m_nodes[idx].get();
		}
	}
	auto& GetAllNodes() const { return m_nodes; }

private:
	std::vector<std::unique_ptr<Node>> m_nodes;

	std::vector<std::unique_ptr<ModelExtend>> m_anims;

}; // SkeletalAnim

}