#ifndef NO_MESHLAB

#pragma once

#include <SM_Vector.h>

#include <vector>
#include <memory>

namespace meshlab { class Sorkine04; }

namespace model
{

struct MeshGeometry;

class MeshIK
{
public:
	MeshIK(MeshGeometry& mesh);
	~MeshIK();

	void PrepareDeform(int main_handle, int handle_region_size, int unconstrained_region_size);

	void Deform(const sm::vec3& translate);

private:
	MeshGeometry& m_mesh;

	std::unique_ptr<meshlab::Sorkine04> m_solver = nullptr;

	std::vector<int> m_handles;
	std::vector<int> m_unconstrained;
	int m_boundary_begin;

	std::vector<uint32_t> m_colors;

}; // MeshIK

}

#endif // NO_MESHLAB