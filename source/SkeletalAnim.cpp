#include "model/SkeletalAnim.h"

namespace model
{

int SkeletalAnim::QueryNodeByName(const std::string& name) const
{
	for (int i = 0, n = m_nodes.size(); i < n; ++i) {
		if (m_nodes[i]->name == name) {
			return i;
		}
	}
	return -1;
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

}