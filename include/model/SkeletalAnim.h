#pragma once

#include <SM_Matrix.h>

#include <boost/noncopyable.hpp>

#include <string>
#include <vector>
#include <memory>

namespace model
{

class SkeletalAnim : boost::noncopyable
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

	struct Animation
	{
		std::string name;

		float duration = 0;

		float ticks_per_second = 0;

		std::vector<std::unique_ptr<NodeAnim>> channels;

	}; // Animation

public:

	int QueryNodeByName(const std::string& name) const;

	void AddAnim(std::unique_ptr<Animation>& anim) {
		m_anims.push_back(std::move(anim));
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

	std::vector<std::unique_ptr<Animation>> m_anims;

}; // SkeletalAnim

}