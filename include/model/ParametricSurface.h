#pragma once

#include "model/Surface.h"

#include <SM_Vector.h>

namespace model
{

struct ParametricInterval
{
	sm::ivec2 divisions;
	sm::vec2  upperBound;
	sm::vec2  texture_count;
};

class ParametricSurface : public Surface
{
public:
	virtual int  GetVertexCount() const override;
	virtual int  GetTriangleIndexCount() const override;

	virtual void GenerateVertices(int vertex_type, std::vector<float>& vertices) const override;
	virtual void GenerateTriangleIndices(std::vector<unsigned short>& indices) const override;

protected:
	void SetInterval(const ParametricInterval& interval);

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const = 0;
	virtual bool InvertNormal(const sm::vec2& domain) const { return false; }

private:
	sm::vec2 ComputeDomain(float i, float j) const;

private:
	sm::ivec2 m_slices;
	sm::ivec2 m_divisions;
	sm::vec2  m_upper_bound;
	sm::vec2  m_texture_count;

}; // ParametricSurface

}
