#pragma once

#include <vector>

namespace model
{

class Adjacencies
{
public:
	static std::vector<unsigned short> Build(const std::vector<unsigned short>& tris);

}; // Adjacencies

}