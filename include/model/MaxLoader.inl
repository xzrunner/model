#pragma once

#include <sstream>

namespace model
{

template<typename T>
void MaxLoader::ParseArray(std::vector<T>& dst, const rapidxml::xml_node<>* src)
{
	int n = dst.size();
	std::stringstream ss(src->value());
	int ptr = 0;
	for (int i = 0; i < n; ++i) {
		ss >> dst[i];
	}
}

}