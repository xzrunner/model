#include "model/BrushBuilder.h"
#include "model/typedef.h"
#include "model/gltf/Model.h"

#include <SM_Calc.h>
#include <SM_Triangulation.h>
#include <polymesh3/Polytope.h>
#include <unirender/Device.h>
#include <unirender/IndexBuffer.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/VertexArray.h>

namespace
{

void dump_vert_buf(model::BrushBuilder::VertexType type,
                   const std::vector<model::BrushBuilder::Vertex>& src,
                   std::vector<float>& dst)
{
    dst.clear();

    switch (type)
    {
    case model::BrushBuilder::VertexType::Pos:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
        }
        break;
    case model::BrushBuilder::VertexType::PosMaterialOffset:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
            dst.push_back(p.mat_id);
            dst.push_back(p.offset);
        }
        break;
    case model::BrushBuilder::VertexType::PosNorm:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.normal.xyz[i]);
            }
        }
        break;
    case model::BrushBuilder::VertexType::PosNormTex:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.normal.xyz[i]);
            }
            for (int i = 0; i < 2; ++i) {
                dst.push_back(p.texcoord.xy[i]);
            }
        }
        break;
    case model::BrushBuilder::VertexType::PosNormTex2:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.normal.xyz[i]);
            }
            for (int i = 0; i < 2; ++i) {
                dst.push_back(p.texcoord.xy[i]);
            }
            for (int i = 0; i < 2; ++i) {
                dst.push_back(p.texcoord2.xy[i]);
            }
        }
        break;
    case model::BrushBuilder::VertexType::PosNormCol:
        for (auto& p : src)
        {
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.pos.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.normal.xyz[i]);
            }
            for (int i = 0; i < 3; ++i) {
                dst.push_back(p.color.xyz[i]);
            }
        }
        break;
    default:
        assert(0);
    }
}

void setup_vert_attr_list(model::BrushBuilder::VertexType type, const std::shared_ptr<ur::VertexBuffer>& vbuf,
                          std::vector<std::shared_ptr<ur::VertexInputAttribute>>& vbuf_attrs)
{
    switch (type)
    {
    case model::BrushBuilder::VertexType::Pos:
        vbuf_attrs.resize(2);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 12
            );
        break;
    case model::BrushBuilder::VertexType::PosMaterialOffset:
        vbuf_attrs.resize(3);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 20
            );
        // mat_id
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 1, 12, 20
            );
        // offset
        vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 1, 16, 20
            );
        break;
    case model::BrushBuilder::VertexType::PosNorm:
        vbuf_attrs.resize(2);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 24
        );
        // normal
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3, 12, 24
        );
        break;
    case model::BrushBuilder::VertexType::PosNormTex:
        vbuf_attrs.resize(3);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 32
        );
        // normal
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3, 12, 32
        );
        // texcoord
        vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 2, 24, 32
        );
        break;
    case model::BrushBuilder::VertexType::PosNormTex2:
        vbuf_attrs.resize(4);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 40
        );
        // normal
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3, 12, 40
        );
        // texcoord
        vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 2, 24, 40
        );
        // texcoord2
        vbuf_attrs[3] = std::make_shared<ur::VertexInputAttribute>(
            3, ur::ComponentDataType::Float, 2, 32, 40
        );

        break;
    case model::BrushBuilder::VertexType::PosNormCol:
        vbuf_attrs.resize(3);
        // pos
        vbuf_attrs[0] = std::make_shared<ur::VertexInputAttribute>(
            0, ur::ComponentDataType::Float, 3, 0, 36
        );
        // normal
        vbuf_attrs[1] = std::make_shared<ur::VertexInputAttribute>(
            1, ur::ComponentDataType::Float, 3, 12, 36
        );
        // color
        vbuf_attrs[2] = std::make_shared<ur::VertexInputAttribute>(
            2, ur::ComponentDataType::Float, 3, 24, 36
        );
        break;
    default:
        assert(0);
    }
}

void setup_geo_vert_type(model::BrushBuilder::VertexType type, unsigned int& vertex_type)
{
    switch (type)
    {
    case model::BrushBuilder::VertexType::PosNormTex:
        vertex_type |= model::VERTEX_FLAG_TEXCOORDS0;
        break;
    case model::BrushBuilder::VertexType::PosNormTex2:
        vertex_type |= model::VERTEX_FLAG_TEXCOORDS0;
        vertex_type |= model::VERTEX_FLAG_TEXCOORDS1;
        break;
    case model::BrushBuilder::VertexType::PosNormCol:
        vertex_type |= model::VERTEX_FLAG_COLOR;
        break;
    }
}

size_t calc_strid(model::BrushBuilder::VertexType type)
{
    switch (type)
    {
    case model::BrushBuilder::VertexType::PosNorm:
        return sizeof(model::BrushBuilder::Vertex::pos)
             + sizeof(model::BrushBuilder::Vertex::normal);
    case model::BrushBuilder::VertexType::PosNormTex:
        return sizeof(model::BrushBuilder::Vertex::pos)
             + sizeof(model::BrushBuilder::Vertex::normal)
             + sizeof(model::BrushBuilder::Vertex::texcoord);
    case model::BrushBuilder::VertexType::PosNormTex2:
        return sizeof(model::BrushBuilder::Vertex::pos)
             + sizeof(model::BrushBuilder::Vertex::normal)
             + sizeof(model::BrushBuilder::Vertex::texcoord)
             + sizeof(model::BrushBuilder::Vertex::texcoord2);
    case model::BrushBuilder::VertexType::PosNormCol:
        return sizeof(model::BrushBuilder::Vertex::pos)
             + sizeof(model::BrushBuilder::Vertex::normal)
             + sizeof(model::BrushBuilder::Vertex::color);
    default:
        assert(0);
        return 0;
    }
}

model::BrushBuilder::Vertex
create_vertex(const sm::vec3& pos, const sm::vec3& normal, const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
              const std::vector<std::vector<std::vector<sm::vec3>>>& colors, size_t i, size_t j, size_t k, sm::cube& aabb)
{
    model::BrushBuilder::Vertex v;

    v.pos = pos;
    aabb.Combine(v.pos);

    v.normal = normal.Normalized();

    if (texcoords.empty()) {
        v.texcoord.Set(0, 0);
    } else {
        v.texcoord = texcoords[i][j][k];
    }

    if (colors.empty()) {
        v.color.Set(1, 1, 1);
    } else {
        v.color = colors[i][j][k];
    }

    return v;
}

}

namespace model
{

std::unique_ptr<BrushModel>
BrushBuilder::BrushFromPolygon(const std::vector<sm::vec3>& polygon)
{
	if (polygon.size() < 3) {
		return nullptr;
	}

    auto fixed_poly = polygon;

	// fix dir
	// should be clockwise, as left-hand system, top face's normal direction is y
	auto v0 = polygon[1] - polygon[0];
	auto v1 = polygon[2] - polygon[1];
	if (v0.Cross(v1).Dot({ 0, 1, 0 }) < 0) {
		std::reverse(fixed_poly.begin(), fixed_poly.end());
	}

	const float dy = 0.1f;
//	const float dy = 30;

	// brush data
    BrushModel::Brush brush;
    brush.desc.mesh_begin = 0;
    brush.desc.mesh_end   = 1;
	int face_num = fixed_poly.size() + 2;
    brush.desc.meshes.push_back({ 0, 0, 0, face_num });

    std::vector<pm3::Polytope::FacePtr> faces;
	faces.reserve(face_num);
	// top
	{
		sm::vec3 tri[3];
		tri[0] = fixed_poly[0];
		tri[1] = fixed_poly[1];
		tri[2] = fixed_poly[2];
		for (int i = 0; i < 3; ++i) {
			tri[i].y += dy;
		}
		auto face = std::make_shared<pm3::Polytope::Face>();
		face->plane = sm::Plane(tri[0], tri[1], tri[2]);
		faces.push_back(face);
	}
	// bottom
	{
		auto face = std::make_shared<pm3::Polytope::Face>();
		face->plane = sm::Plane(fixed_poly[2], fixed_poly[1], fixed_poly[0]);
		faces.push_back(face);
	}
	// edge faces
	for (size_t i = 0, n = fixed_poly.size(); i < n; ++i)
	{
		auto& v0 = fixed_poly[i];
		auto& v1 = fixed_poly[(i + 1) % fixed_poly.size()];
		auto face = std::make_shared<pm3::Polytope::Face>();
		face->plane = sm::Plane(v0, v1, { v1.x, v1.y + dy, v1.z });
		faces.push_back(face);
	}
    brush.impl = std::make_shared<pm3::Polytope>(faces);

    auto model_model = std::make_unique<BrushModel>();
    std::vector<BrushModel::Brush> brushes;
    brushes.push_back(brush);
    model_model->SetBrushes(brushes);

    return model_model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPN(const ur::Device& dev, const std::vector<pm3::PolytopePtr>& brushes)
{
    return PolymeshFromBrush(dev, VertexType::PosNorm, brushes, std::vector<std::vector<std::vector<sm::vec2>>>(), std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPN(const ur::Device& dev, const model::BrushModel& brush_model)
{
    return PolymeshFromBrush(dev, VertexType::PosNorm, brush_model, std::vector<std::vector<std::vector<sm::vec2>>>(), std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNT(const ur::Device& dev, const std::vector<pm3::PolytopePtr>& brushes, const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
{
    return PolymeshFromBrush(dev, VertexType::PosNormTex, brushes, texcoords, std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNT(const ur::Device& dev, const model::BrushModel& brush_model, const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
{
    return PolymeshFromBrush(dev, VertexType::PosNormTex, brush_model, texcoords, std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNC(const ur::Device& dev, const std::vector<pm3::PolytopePtr>& brushes, const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    return PolymeshFromBrush(dev, VertexType::PosNormCol, brushes, std::vector<std::vector<std::vector<sm::vec2>>>(), colors);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNC(const ur::Device& dev, const model::BrushModel& brush_model, const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    return PolymeshFromBrush(dev, VertexType::PosNormCol, brush_model, std::vector<std::vector<std::vector<sm::vec2>>>(), colors);
}

void BrushBuilder::PolymeshFromBrush(const ur::Device& dev, const std::vector<std::shared_ptr<pm3::Polytope>>& src, 
                                     const std::vector<int>& materials, const std::vector<float>& offsets, gltf::Model& dst)
{
    auto model = std::make_unique<Model>(&dev);

	std::unique_ptr<Model::Mesh> mesh = nullptr;

	mesh = std::make_unique<Model::Mesh>();

	auto mat = std::make_unique<Model::Material>();
	int mat_idx = model->materials.size();
	mesh->material = mat_idx;
	mat->diffuse_tex = -1;
	model->materials.push_back(std::move(mat));

	std::vector<Vertex> vertices;
    std::vector<unsigned short> indices;

    assert(src.size() == materials.size() && src.size() == offsets.size());

	sm::cube aabb;
	int start_idx = 0;
    for (int i = 0, n = src.size(); i < n; ++i)
    {
        auto& b = src[i];

        auto& points = b->Points();
        for (auto& p : points) 
        {
            auto vert = create_vertex(p->pos, sm::vec3(), {}, {}, 0, 0, 0, aabb);
            vert.mat_id = materials.empty() ? 0 : materials[i];
            vert.offset = offsets.empty() ? 0 : offsets[i];
            vertices.push_back(vert);
        }

        auto& faces  = b->Faces();
        for (auto& f : faces) {
            auto tris_idx = Triangulation(points, f->border, f->holes);
            for (auto& idx : tris_idx) {
                indices.push_back(start_idx + idx);
            }
        }

        start_idx += points.size();
    }
    if (vertices.empty()) {
        return;
    }

    auto va = dev.CreateVertexArray();

    std::vector<float> buf;
    dump_vert_buf(VertexType::PosMaterialOffset, vertices, buf);

    auto vbuf_sz = sizeof(float) * buf.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(buf.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, 0);
    auto ibuf_sz = sizeof(unsigned short) * indices.size();
    ibuf->SetCount(indices.size());
    ibuf->Reserve(ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
    va->SetIndexBuffer(ibuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    setup_vert_attr_list(VertexType::PosMaterialOffset, vbuf, vbuf_attrs);
    va->SetVertexBufferAttrs(vbuf_attrs);

    auto d_prim = std::make_shared<gltf::Primitive>();
    d_prim->va = va;
    d_prim->size = aabb.Size();
    d_prim->material = std::make_shared<gltf::Material>();
    auto d_mesh = std::make_shared<gltf::Mesh>();
    d_mesh->primitives.push_back(d_prim);
    auto d_node = std::make_shared<gltf::Node>();
    d_node->mesh = d_mesh;
    auto d_scene = std::make_shared<gltf::Scene>();
    d_scene->nodes.push_back(d_node);
    dst.scenes.push_back(d_scene);

    dst.scene = d_scene;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(const ur::Device& dev, VertexType type, const std::vector<pm3::PolytopePtr>& brushes,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
                                const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    auto model = std::make_unique<Model>(&dev);

	std::unique_ptr<Model::Mesh> mesh = nullptr;
	std::unique_ptr<Model::Mesh> border_mesh = nullptr;

	mesh = std::make_unique<Model::Mesh>();

	border_mesh = std::make_unique<Model::Mesh>();

	auto mat = std::make_unique<Model::Material>();
	int mat_idx = model->materials.size();
	mesh->material = mat_idx;
	border_mesh->material = mat_idx;
	mat->diffuse_tex = -1;
	model->materials.push_back(std::move(mat));

	std::vector<Vertex> vertices;
	std::vector<Vertex> border_vertices;
	std::vector<unsigned short> border_indices;

	sm::cube aabb;
	int start_idx = 0;
    for (int i = 0, n = brushes.size(); i < n; ++i)
    {
        auto& b = brushes[i];
        auto& faces  = b->Faces();
        auto& points = b->Points();
        for (int j = 0, m = faces.size(); j < m; ++j)
	    {
            auto& f = faces[j];
            auto& norm = f->plane.normal;
            auto tris_idx = Triangulation(points, f->border, f->holes);
            for (auto& idx : tris_idx) {
                vertices.push_back(create_vertex(points[idx]->pos, norm, texcoords, colors, i, j, idx, aabb));
            }
            for (size_t k = 0, l = f->border.size(); k < l; ++k) {
			    border_vertices.push_back(create_vertex(points[f->border[k]]->pos, norm, texcoords, colors, i, j, k, aabb));
		    }
		    for (int k = 0, n = f->border.size() - 1; k < n; ++k) {
			    border_indices.push_back(start_idx + k);
			    border_indices.push_back(start_idx + k + 1);
		    }
		    border_indices.push_back(static_cast<unsigned short>(start_idx + f->border.size() - 1));
		    border_indices.push_back(start_idx);
		    start_idx += f->border.size();
	    }
    }
    if (!vertices.empty()) {
        FlushVertices(dev, type, mesh, border_mesh, vertices, border_vertices, border_indices, *model);
    }
	model->aabb = aabb;

	return model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(const ur::Device& dev, VertexType type, const model::BrushModel& brush_model,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
                                const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    auto& src_brushes = brush_model.GetBrushes();
    std::vector<pm3::PolytopePtr> brushes;
    brushes.reserve(src_brushes.size());
    for (auto& b : src_brushes) {
        brushes.push_back(b.impl);
    }
    return PolymeshFromBrush(dev, type, brushes, texcoords, colors);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromPolygon(const ur::Device& dev, const std::vector<sm::vec3>& polygon)
{
    auto brush_model = model::BrushBuilder::BrushFromPolygon(polygon);
    auto& brushes = brush_model->GetBrushes();
    assert(brushes.size() == 1);
    const size_t p_num = brushes[0].impl->Points().size();
    const size_t f_num = brushes[0].impl->Faces().size();
    std::vector<sm::vec2> texcoords(p_num, sm::vec2(0, 0));
    std::vector<sm::vec3> colors(p_num, sm::vec3(1, 1, 1));
    auto model = model::BrushBuilder::PolymeshFromBrush(dev, VertexType::PosNorm, *brush_model,
        {{ f_num, texcoords }}, {{ f_num, colors }});
    model->ext = std::move(brush_model);

    return model;
}

void BrushBuilder::UpdateVBO(Model& model, const BrushModel::Brush& brush)
{
    if (!brush.impl) {
        return;
    }

    static const sm::vec3 WHITE(1, 1, 1);

	auto& faces = brush.impl->Faces();
    auto& points = brush.impl->Points();
	assert(brush.desc.mesh_end - brush.desc.mesh_begin == brush.desc.meshes.size());
	// traverse brush's meshes
	for (int i = brush.desc.mesh_begin; i < brush.desc.mesh_end; ++i)
	{
		// gen mesh's vertex buffer
		std::vector<Vertex> vertices, border_vertices;
		for (auto& src_mesh : brush.desc.meshes)
		{
			int tex_w = src_mesh.tex_width;
			int tex_h = src_mesh.tex_height;
			sm::cube aabb; // todo
			for (int j = src_mesh.face_begin; j < src_mesh.face_end; ++j)
			{
				auto& f = faces[j];
                auto tris_idx = Triangulation(points, f->border, f->holes);
                for (auto& idx : tris_idx) {
                    vertices.push_back(CreateVertex(f, points[idx]->pos, tex_w, tex_h, WHITE, aabb));
                }
				assert(f->border.size() > 2);
				for (auto& vert : f->border) {
					border_vertices.push_back(CreateVertex(f, points[vert]->pos, tex_w, tex_h, WHITE, aabb));
				}
			}
		}

        std::vector<float> vertex_buf, border_vertex_buf;
        dump_vert_buf(VertexType::PosNormTex, vertices, vertex_buf);
        dump_vert_buf(VertexType::PosNormTex, border_vertices, border_vertex_buf);

		// upload buffer data
        auto vb = model.meshes[i]->geometry.vertex_array->GetVertexBuffer();
        vb->ReadFromMemory(vertex_buf.data(), sizeof(float) * vertex_buf.size(), 0);
        auto vb_border = model.border_meshes[i]->geometry.vertex_array->GetVertexBuffer();
        vb_border->ReadFromMemory(border_vertex_buf.data(), sizeof(float) * border_vertex_buf.size(), 0);
	}
}

void BrushBuilder::UpdateVBO(Model& model, const model::BrushModel& brush_model)
{
    for (auto& brush : brush_model.GetBrushes()) {
        UpdateVBO(model, brush);
    }
}

void BrushBuilder::CreateMeshRenderBuf(const ur::Device& dev, VertexType type, model::Model::Mesh& mesh,
                                       const std::vector<Vertex>& vertices)
{
    auto va = dev.CreateVertexArray();

    std::vector<float> buf;
    dump_vert_buf(type, vertices, buf);

    auto vbuf_sz = sizeof(float) * buf.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(buf.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    setup_vert_attr_list(type, vbuf, vbuf_attrs);
    va->SetVertexBufferAttrs(vbuf_attrs);

    mesh.geometry.vertex_array = va;
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(false, vertices.size(), 0));
    setup_geo_vert_type(type, mesh.geometry.vertex_type);
}

void BrushBuilder::CreateBorderMeshRenderBuf(const ur::Device& dev, VertexType type, model::Model::Mesh& mesh,
                                             const std::vector<Vertex>& vertices,
                                             const std::vector<unsigned short>& indices)
{
    auto va = dev.CreateVertexArray();

    std::vector<float> buf;
    dump_vert_buf(type, vertices, buf);

    auto usage = ur::BufferUsageHint::StaticDraw;

    auto ibuf_sz = sizeof(unsigned short) * indices.size();
    auto ibuf = dev.CreateIndexBuffer(usage, ibuf_sz);
    ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
    va->SetIndexBuffer(ibuf);

    auto vbuf_sz = sizeof(float) * vertices.size();
    auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
    vbuf->ReadFromMemory(buf.data(), vbuf_sz, 0);
    va->SetVertexBuffer(vbuf);

    std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;
    setup_vert_attr_list(type, vbuf, vbuf_attrs);
    va->SetVertexBufferAttrs(vbuf_attrs);

    mesh.geometry.vertex_array = va;
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(true, indices.size(), 0));
    setup_geo_vert_type(type, mesh.geometry.vertex_type);
}

BrushBuilder::Vertex
BrushBuilder::CreateVertex(const pm3::Polytope::FacePtr& face, const sm::vec3& pos, int tex_w, int tex_h,
                           const sm::vec3& color, sm::cube& aabb)
{
    Vertex v;

    v.pos = pos;
    aabb.Combine(v.pos);

    v.normal = face->plane.normal;

    if (tex_w == 0 || tex_h == 0) {
        v.texcoord.Set(0, 0);
    } else {
        v.texcoord = face->tex_map.CalcTexCoords(
            pos, static_cast<float>(tex_w), static_cast<float>(tex_h));
    }

    v.color = color;

    return v;
}

void BrushBuilder::FlushVertices(const ur::Device& dev, VertexType type,
                                 std::unique_ptr<model::Model::Mesh>& mesh,
                                 std::unique_ptr<model::Model::Mesh>& border_mesh,
                                 std::vector<Vertex>& vertices,
                                 std::vector<Vertex>& border_vertices,
                                 std::vector<unsigned short>& border_indices,
                                 model::Model& dst)
{
    CreateMeshRenderBuf(dev, type, *mesh, vertices);
    dst.meshes.push_back(std::move(mesh));
    vertices.clear();

    CreateBorderMeshRenderBuf(dev, type, *border_mesh, border_vertices, border_indices);
    dst.border_meshes.push_back(std::move(border_mesh));
    border_vertices.clear();
}

std::vector<size_t>
BrushBuilder::Triangulation(const std::vector<pm3::Polytope::PointPtr>& verts,
                            const std::vector<size_t>& border, const std::vector<std::vector<size_t>>& holes)
{
    std::vector<sm::vec3> border3;
    border3.reserve(border.size());
    for (auto& idx : border) {
        border3.push_back(verts[idx]->pos);
    }
    auto norm = sm::calc_face_normal(border3);
    auto rot = sm::mat4(sm::Quaternion::CreateFromVectors(norm, sm::vec3(0, 1, 0)));

    std::map<sm::vec2, size_t> pos2idx;
    auto trans_loop3to2 = [&](const std::vector<pm3::Polytope::PointPtr>& verts,
                              const std::vector<size_t>& loop3) -> std::vector<sm::vec2>
    {
        std::vector<sm::vec2> loop2;
        loop2.reserve(loop3.size());
        for (auto& idx : loop3)
        {
            auto& pos3 = verts[idx]->pos;
            auto p3_rot = rot * pos3;
            sm::vec2 pos2(p3_rot.x, p3_rot.z);
            auto status = pos2idx.insert({ pos2, idx });
            assert(status.second);

            loop2.push_back(pos2);
        }
        return loop2;
    };

    auto border2 = trans_loop3to2(verts, border);

    std::vector<std::vector<sm::vec2>> holes2;
    holes2.resize(holes.size());
    for (size_t i = 0, n = holes.size(); i < n; ++i) {
        holes2[i] = trans_loop3to2(verts, holes[i]);
    }

    std::vector<size_t> ret;

    std::vector<sm::vec2> tris;
    sm::triangulate_holes(border2, holes2, tris);
    assert(tris.size() % 3 == 0);
    ret.reserve(tris.size());
    for (size_t i = 0, n = tris.size(); i < n; )
    {
        std::vector<sm::vec2*> tri(3);
        for (size_t j = 0; j < 3; ++j) {
            tri[j] = &tris[i++];
        }
        if (sm::is_turn_left(*tri[0], *tri[1], *tri[2])) {
            std::reverse(tri.begin(), tri.end());
        }
        for (size_t j = 0; j < 3; ++j)
        {
            auto itr = pos2idx.find(*tri[j]);
            assert(itr != pos2idx.end());
            ret.push_back(itr->second);
        }
    }

    return ret;
}

}