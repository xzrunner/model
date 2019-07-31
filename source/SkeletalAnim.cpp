#include "model/SkeletalAnim.h"

namespace model
{

std::unique_ptr<model::ModelExtend> SkeletalAnim::Clone() const
{
    auto ret = std::make_unique<SkeletalAnim>();

    ret->m_nodes.reserve(m_nodes.size());
    for (auto& n : m_nodes) {
        ret->m_nodes.push_back(std::make_unique<Node>(*n));
    }

    ret->m_tpose_world_trans = m_tpose_world_trans;

    ret->m_anims.reserve(m_anims.size());
    for (auto& anim : m_anims) {
        ret->m_anims.push_back(std::make_unique<SkeletalAnim::ModelExtend>(*anim));
    }

    return ret;
}

void SkeletalAnim::SetAnims(std::vector<std::unique_ptr<ModelExtend>>& anims)
{
	m_anims = std::move(anims);
}

void SkeletalAnim::SetNodes(std::vector<std::unique_ptr<Node>>& nodes)
{
	m_nodes = std::move(nodes);
	InitTPoseTrans();
}

void SkeletalAnim::PrintNodeTree() const
{
	printf("-----------------------------------\n");
	for (size_t i = 0; i < m_nodes.size(); ++i)
	{
		printf("%d: ", i);
		int parent = m_nodes[i]->parent;
		while (parent != -1) {
			printf("%d ", parent);
			parent = m_nodes[parent]->parent;
		}
		printf("\n");
	}
	printf("-----------------------------------\n");
}

void SkeletalAnim::InitTPoseTrans()
{
	std::vector<sm::mat4> tpose_local_trans;
	tpose_local_trans.resize(m_nodes.size());
	for (size_t i = 0; i < m_nodes.size(); ++i)
	{
		sm::vec3 pos, rot, scale;
		m_nodes[i]->local_trans.Decompose(pos, rot, scale);

		auto& d = tpose_local_trans[i];
        d.c[0][0] = scale.x; d.c[1][0] = 0;       d.c[2][0] = 0;       d.c[3][0] = pos.x;
        d.c[0][1] = 0;       d.c[1][1] = scale.y; d.c[2][1] = 0;       d.c[3][1] = pos.y;
        d.c[0][2] = 0;       d.c[1][2] = 0;       d.c[2][2] = scale.z; d.c[3][2] = pos.z;
        d.c[0][3] = 0;       d.c[1][3] = 0;       d.c[2][3] = 0;       d.c[3][3] = 1;
	}

	m_tpose_world_trans.resize(m_nodes.size());
	for (size_t i = 0; i < m_nodes.size(); ++i)
	{
		auto g_trans = tpose_local_trans[i];
		int parent = m_nodes[i]->parent;
		while (parent != -1) {
			g_trans = g_trans * tpose_local_trans[parent];
			parent = m_nodes[parent]->parent;
		}
		m_tpose_world_trans[i] = g_trans;
	}
}

}