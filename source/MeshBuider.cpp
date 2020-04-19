#include "model/MeshBuider.h"
#include "model/typedef.h"

#include <unirender2/Device.h>
#include <unirender2/VertexBuffer.h>
#include <unirender2/VertexBufferAttribute.h>
#include <unirender2/VertexArray.h>

namespace model
{

std::unique_ptr<Model::Mesh>
MeshBuider::CreateCube(const ur2::Device& dev, const sm::vec3& half_extents)
{
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

    auto va = dev.CreateVertexArray();

    auto vbuf_sz = sizeof(vertices);
    auto vbuf = dev.CreateVertexBuffer(ur2::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(vertices, vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur2::VertexBufferAttribute>> vbuf_attrs;
    vbuf_attrs.resize(3);
    vbuf_attrs[0] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 0, 32
    );
    vbuf_attrs[1] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 3, 12, 32
    );
    vbuf_attrs[2] = std::make_shared<ur2::VertexBufferAttribute>(
        ur2::ComponentDataType::Float, 2, 24, 32
    );
    va->SetVertexBufferAttrs(vbuf_attrs);

    auto mesh = std::make_unique<Model::Mesh>();
    mesh->geometry.vertex_array = va;
    int v_num = sizeof(vertices) / sizeof(vertices[0]) / 8;
	mesh->geometry.sub_geometries.push_back(SubmeshGeometry(false, v_num, 0));
	mesh->geometry.sub_geometry_materials.push_back(0);
	mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
	mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
	mesh->material = 0;

    return mesh;
}

}