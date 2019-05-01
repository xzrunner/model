#include "model/MeshBuider.h"
#include "model/typedef.h"

#include <unirender/Blackboard.h>
#include <unirender/RenderContext.h>

namespace model
{

std::unique_ptr<Model::Mesh>
MeshBuider::CreateCube(const sm::vec3& half_extents)
{
    auto mesh = std::make_unique<Model::Mesh>();

	ur::RenderContext::VertexInfo vi;

    auto& sz = half_extents;
    float vertices[] = {
        // back face
        -sz.x, -sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         sz.x,  sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         sz.x, -sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
         sz.x,  sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -sz.x, -sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -sz.x,  sz.y, -sz.z,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -sz.x, -sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         sz.x, -sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         sz.x,  sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         sz.x,  sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -sz.x,  sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -sz.x, -sz.y,  sz.z,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -sz.x,  sz.y,  sz.z, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -sz.x,  sz.y, -sz.z, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -sz.x, -sz.y, -sz.z, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -sz.x, -sz.y, -sz.z, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -sz.x, -sz.y,  sz.z, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -sz.x,  sz.y,  sz.z, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
        sz.x,  sz.y,  sz.z,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
        sz.x, -sz.y, -sz.z,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
        sz.x,  sz.y, -sz.z,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
        sz.x, -sz.y, -sz.z,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
        sz.x,  sz.y,  sz.z,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
        sz.x, -sz.y,  sz.z,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
        // bottom face
        -sz.x, -sz.y, -sz.z,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         sz.x, -sz.y, -sz.z,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
         sz.x, -sz.y,  sz.z,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         sz.x, -sz.y,  sz.z,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -sz.x, -sz.y,  sz.z,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -sz.x, -sz.y, -sz.z,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -sz.x,  sz.y, -sz.z,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         sz.x,  sz.y , sz.z,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         sz.x,  sz.y, -sz.z,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
         sz.x,  sz.y,  sz.z,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -sz.x,  sz.y, -sz.z,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -sz.x,  sz.y,  sz.z,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
    };

    const size_t stride = 8;    // float number
	vi.vn = sizeof(vertices) / sizeof(vertices[0]) / stride;
	vi.vertices = &vertices[0];
	vi.stride = stride * sizeof(float);

	vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 32, 0));   // pos
	vi.va_list.push_back(ur::VertexAttrib("normal",   3, 4, 32, 12));  // normal
	vi.va_list.push_back(ur::VertexAttrib("texcoord", 2, 4, 32, 24));  // texcoord

	ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
		vi, mesh->geometry.vao, mesh->geometry.vbo, mesh->geometry.ebo);
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, vi.vn, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;

    return mesh;
}

}