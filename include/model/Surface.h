#pragma once

#include <vector>

namespace model
{

class Surface
{
public:
	virtual const char* Type() const = 0;
	virtual int GetVertexCount() const = 0;
	virtual int GetTriangleIndexCount() const = 0;
	virtual void GenerateVertices(int vertex_type, std::vector<float>& vertices) const = 0;
	virtual void GenerateTriangleIndices(std::vector<unsigned short>& indices) const = 0;
	virtual ~Surface() {}
};

}