#include "model/ParametricSurface.h"
#include "model/typedef.h"

namespace model
{

int ParametricSurface::GetVertexCount() const
{
	return m_divisions.x * m_divisions.y;
}

int ParametricSurface::GetTriangleIndexCount() const
{
	return 6 * m_slices.x * m_slices.y;
}

void ParametricSurface::GenerateVertices(int vertex_type, std::vector<float>& vertices) const
{
	int stride = 3;
	if (vertex_type & VERTEX_FLAG_NORMALS) {
		stride += 3;
	}
	if (vertex_type & VERTEX_FLAG_TEXCOORDS) {
		stride += 2;
	}

	vertices.resize(GetVertexCount() * stride);
	float* ptr = &vertices[0];

	for (int j = 0; j < m_divisions.y; j++)
	{
		for (int i = 0; i < m_divisions.x; i++)
		{

			// Compute Position
			sm::vec2 domain = ComputeDomain(static_cast<float>(i), static_cast<float>(j));
			sm::vec3 range = Evaluate(domain);
			memcpy(ptr, &range, sizeof(float) * 3);
			ptr += 3;

			// Compute Normal
			if (vertex_type & VERTEX_FLAG_NORMALS)
			{
				float s = static_cast<float>(i), t = static_cast<float>(j);

				// Nudge the point if the normal is indeterminate.
				if (i == 0) s += 0.01f;
				if (i == m_divisions.x - 1) s -= 0.01f;
				if (j == 0) t += 0.01f;
				if (j == m_divisions.y - 1) t -= 0.01f;

				// Compute the tangents and their cross product.
				sm::vec3 p = Evaluate(ComputeDomain(s, t));
				sm::vec3 u = Evaluate(ComputeDomain(s + 0.01f, t)) - p;
				sm::vec3 v = Evaluate(ComputeDomain(s, t + 0.01f)) - p;
				sm::vec3 normal = u.Cross(v).Normalized();
				if (InvertNormal(domain)) {
					normal = -normal;
				}
				memcpy(ptr, &normal, sizeof(float) * 3);
				ptr += 3;
			}

			// Compute Texture Coordinates
			if (vertex_type & VERTEX_FLAG_TEXCOORDS)
			{
				float s = /*m_texture_count.x * */i / static_cast<float>(m_slices.x);
				float t = /*m_texture_count.y * */j / static_cast<float>(m_slices.y);
				(*ptr++) = s;
				(*ptr++) = t;
			}
		}
	}
}

void
ParametricSurface::GenerateTriangleIndices(std::vector<unsigned short>& indices) const
{
	indices.resize(GetTriangleIndexCount());
	std::vector<unsigned short>::iterator index = indices.begin();
	for (int j = 0, vertex = 0; j < m_slices.y; j++) {
		for (int i = 0; i < m_slices.x; i++) {
			int next = (i + 1) % m_divisions.x;
			*index++ = vertex + i;
			*index++ = vertex + next;
			*index++ = vertex + i + m_divisions.x;
			*index++ = vertex + next;
			*index++ = vertex + next + m_divisions.x;
			*index++ = vertex + i + m_divisions.x;
		}
		vertex += m_divisions.x;
	}
}

void ParametricSurface::SetInterval(const ParametricInterval& interval)
{
	m_divisions = interval.divisions;
	m_upper_bound = interval.upperBound;
	m_texture_count = interval.texture_count;
	m_slices = m_divisions - sm::ivec2(1, 1);
}

sm::vec2 ParametricSurface::ComputeDomain(float x, float y) const
{
	return sm::vec2(x * m_upper_bound.x / m_slices.x, y * m_upper_bound.y / m_slices.y);
}

}