#include "model/Mesh.h"

namespace model
{

Mesh::Mesh()
	: m_vertex_type(0)
	, m_index_count(0)
{
}

Mesh::~Mesh()
{
}

void Mesh::SetRenderBuffer(int vertex_type, const std::vector<float>& vertices,
						   const std::vector<uint16_t>& indices)
{
	m_vertex_type = vertex_type;
	m_vertices = vertices;
	m_indices = indices;
}

}