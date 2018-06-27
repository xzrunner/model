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

}