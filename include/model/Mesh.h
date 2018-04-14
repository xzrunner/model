#pragma once

#include "model/Material.h"

#include <boost/noncopyable.hpp>

#include <string>
#include <vector>
#include <memory>

#include <stdint.h>

namespace model
{

class Mesh : boost::noncopyable
{
public:
	Mesh();
	virtual ~Mesh();

	void SetType(const std::string& type) { m_type = type; }
	const std::string& GetType() const { return m_type; }

	void SetRenderBuffer(int vertex_type, const std::vector<float>& vertices,
		const std::vector<uint16_t>& indices);

	void SetIndexCount(int count) { m_index_count = count; }

	void SetMaterial(const Material& material) { m_material = material; }
	const Material& GetMaterial() const { return m_material; }

	int GetVertexType() const { return m_vertex_type; }
	const std::vector<float>& GetVertices() const { return m_vertices; }

	const std::vector<uint16_t>& GetIndices() const { return m_indices; }

private:
	std::string m_type;

	int m_vertex_type;
	std::vector<float> m_vertices;

	std::vector<uint16_t> m_indices;

	int m_index_count;

	Material m_material;

}; // Mesh

using MeshPtr = std::shared_ptr<Mesh>;

}
