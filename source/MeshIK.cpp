#ifndef NO_MESHLAB

#include "model/MeshIK.h"
#include "model/MeshGeometry.h"

#include <meshlab/Sorkine04.h>
#include <unirender/VertexArray.h>
#include <unirender/VertexBuffer.h>

namespace
{

std::vector<std::vector<int>>
build_adjacent(const model::MeshGeometry& mesh)
{
	std::vector<std::vector<int>> adj;

	auto& rd = mesh.raw_data;

	for (auto& p : rd->vertices) {
		adj.push_back(std::vector<int>());
	}
	for (auto& f : rd->faces)
	{
		for (int i = 0, n = f.size(); i < n; ++i) {
			int a = f[i];
			int b = f[(i + 1) % n];
			adj[a].push_back(b);
		}
	}

	return adj;
}

void select_handles(
	const std::vector<std::vector<int>>& adj, int num_vertices, int main_handle, int handle_region_size,
	int unconstrained_region_size, std::vector<int>& handles, std::vector<int>& unconstrained, int& boundary_begin)
{
	std::vector<int> curr_ring;
	curr_ring.push_back(main_handle);

	std::vector<bool> visited;
	visited.resize(num_vertices, false);

	//std::vector<bool> unconstrained_set;
	//std::vector<bool> handles_set;
	//unconstrained_set.resize(num_vertices, false);
	//handles_set.resize(num_vertices, false);

	for (int k = 0; k < handle_region_size; ++k)
	{
		std::vector<int> next_ring;
		for (size_t i = 0; i < curr_ring.size(); ++i)
		{
			int e = curr_ring[i];
			if (visited[e]) {
				continue;
			}

			handles.push_back(e);
			visited[e]     = true;
			//handles_set[e] = true;

			auto& adjs = adj[e];
			for (size_t j = 0; j < adjs.size(); ++j) {
				next_ring.push_back(adjs[j]);
			}
		}
		curr_ring = next_ring;
	}

	for (int k = 0; k < unconstrained_region_size; ++k)
	{
		std::vector<int> next_ring;
		for (size_t i = 0; i < curr_ring.size(); ++i)
		{
			int e = curr_ring[i];
			if (visited[e]) {
				continue;
			}

			unconstrained.push_back(e);
			visited[e]           = true;
			//unconstrained_set[e] = true;

			auto& adjs = adj[e];
			for (size_t j = 0; j < adjs.size(); ++j) {
				next_ring.push_back(adjs[j]);
			}
		}
		curr_ring = next_ring;
	}


	boundary_begin = static_cast<int>(handles.size());
	for (size_t i = 0; i < curr_ring.size(); ++i)
	{
		int e = curr_ring[i];
		if (visited[e]) {
			continue;
		}
		handles.push_back(e);
		visited[e] = true;
	}
}

}

namespace model
{

MeshIK::MeshIK(MeshGeometry& mesh)
	: m_mesh(mesh)
{
}

MeshIK::~MeshIK()
{
}

void MeshIK::PrepareDeform(int main_handle, int handle_region_size, int unconstrained_region_size)
{
	auto& rd = m_mesh.raw_data;

	std::vector<std::vector<int>> adj = build_adjacent(m_mesh);
	select_handles(adj, rd->vertices.size(), main_handle, handle_region_size, unconstrained_region_size, m_handles, m_unconstrained, m_boundary_begin);

	// init colors
	m_colors.resize(rd->vertices.size(), 0xffffffff);
	for (auto& i : m_handles) {
		m_colors[i] = 0xff005555;
	}
	for (auto& i : m_unconstrained) {
		m_colors[i] = 0xff550000;
	}

	// put all handles and unconstrained in one array, and send to solver.
	std::vector<int> combined;
	combined.reserve(m_handles.size() + m_unconstrained.size());
	std::copy(m_handles.begin(), m_handles.end(), std::back_inserter(combined));
	std::copy(m_unconstrained.begin(), m_unconstrained.end(), std::back_inserter(combined));

	std::vector<int> cells;
	int n = 0;
	for (auto& f : rd->faces) {
		assert(f.size() == 3);
		n += f.size();
	}
	cells.reserve(n);
	for (auto& f : rd->faces) {
		std::copy(f.begin(), f.end(), std::back_inserter(cells));
	}

	m_solver = std::make_unique<meshlab::Sorkine04>(cells, &rd->vertices.data()->x, rd->vertices.size() * 3, combined, m_handles.size(), true);
}

void MeshIK::Deform(const sm::vec3& translate)
{
	auto& rd = m_mesh.raw_data;

	// move
	std::vector<sm::vec3> arr;
	arr.resize(m_handles.size());
	for (int i = 0; i < m_boundary_begin; ++i) {
		arr[i] = rd->vertices[m_handles[i]] + translate;
	}
	for (int i = m_boundary_begin, n = m_handles.size(); i < n; ++i) {
		arr[i] = rd->vertices[m_handles[i]];
	}

	m_solver->doDeform(&arr.data()->x, m_handles.size(), &rd->vertices.data()->x);

	// update mesh data
	rd->CalcNormals();

	size_t buf_sz = rd->vertices.size() * (sizeof(float) * 3 * 2 + sizeof(uint32_t));
	uint8_t* buf = new uint8_t[buf_sz];
	uint8_t* ptr = buf;
	for (int i = 0, n = rd->vertices.size(); i < n; ++i)
	{
		memcpy(ptr, &rd->vertices[i].x, sizeof(float) * 3);
		ptr += sizeof(float) * 3;

		memcpy(ptr, &rd->normals[i].x, sizeof(float) * 3);
		ptr += sizeof(float) * 3;

		memcpy(ptr, &m_colors[i], sizeof(uint32_t));
		ptr += sizeof(uint32_t);
	}

    m_mesh.vertex_array->GetVertexBuffer()->ReadFromMemory(buf, buf_sz, 0);
	delete[] buf;
}

}

#endif // NO_MESHLAB