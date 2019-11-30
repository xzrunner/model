#include "model/BrushBuilder.h"
#include "model/typedef.h"

#include <polymesh3/Geometry.h>
#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

namespace
{

void dump_vert_buf(model::BrushBuilder::VertexType type,
                   const std::vector<model::BrushBuilder::Vertex>& src,
                   std::vector<float>& dst)
{
    dst.clear();

    switch (type)
    {
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

void setup_vert_attr_list(model::BrushBuilder::VertexType type, ur::RenderContext::VertexInfo& vi)
{
    switch (type)
    {
    case model::BrushBuilder::VertexType::PosNorm:
	    vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 24, 0));   // pos
	    vi.va_list.push_back(ur::VertexAttrib("normal",   3, 4, 24, 12));  // normal
        break;
    case model::BrushBuilder::VertexType::PosNormTex:
	    vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 32, 0));   // pos
	    vi.va_list.push_back(ur::VertexAttrib("normal",   3, 4, 32, 12));  // normal
	    vi.va_list.push_back(ur::VertexAttrib("texcoord", 2, 4, 32, 24));  // texcoord
        break;
    case model::BrushBuilder::VertexType::PosNormCol:
	    vi.va_list.push_back(ur::VertexAttrib("pos",      3, 4, 36, 0));   // pos
	    vi.va_list.push_back(ur::VertexAttrib("normal",   3, 4, 36, 12));  // normal
        vi.va_list.push_back(ur::VertexAttrib("color",    3, 4, 36, 24));  // color
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
        vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
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

    std::vector<pm3::FacePtr> faces;
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
		auto face = std::make_shared<pm3::Face>();
		face->plane = sm::Plane(tri[0], tri[1], tri[2]);
		faces.push_back(face);
	}
	// bottom
	{
		auto face = std::make_shared<pm3::Face>();
		face->plane = sm::Plane(fixed_poly[2], fixed_poly[1], fixed_poly[0]);
		faces.push_back(face);
	}
	// edge faces
	for (size_t i = 0, n = fixed_poly.size(); i < n; ++i)
	{
		auto& v0 = fixed_poly[i];
		auto& v1 = fixed_poly[(i + 1) % fixed_poly.size()];
		auto face = std::make_shared<pm3::Face>();
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
BrushBuilder::PolymeshFromBrushPN(const std::vector<pm3::PolytopePtr>& brushes)
{
    return PolymeshFromBrush(VertexType::PosNorm, brushes, std::vector<std::vector<std::vector<sm::vec2>>>(), std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPN(const model::BrushModel& brush_model)
{
    return PolymeshFromBrush(VertexType::PosNorm, brush_model, std::vector<std::vector<std::vector<sm::vec2>>>(), std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNT(const std::vector<pm3::PolytopePtr>& brushes, const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
{
    return PolymeshFromBrush(VertexType::PosNormTex, brushes, texcoords, std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNT(const model::BrushModel& brush_model, const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
{
    return PolymeshFromBrush(VertexType::PosNormTex, brush_model, texcoords, std::vector<std::vector<std::vector<sm::vec3>>>());
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNC(const std::vector<pm3::PolytopePtr>& brushes, const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    return PolymeshFromBrush(VertexType::PosNormCol, brushes, std::vector<std::vector<std::vector<sm::vec2>>>(), colors);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrushPNC(const model::BrushModel& brush_model, const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    return PolymeshFromBrush(VertexType::PosNormCol, brush_model, std::vector<std::vector<std::vector<sm::vec2>>>(), colors);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(VertexType type, const std::vector<pm3::PolytopePtr>& brushes,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
                                const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    auto model = std::make_unique<Model>();

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
		    for (size_t k = 1; k < f->points.size() - 1; ++k)
		    {
			    vertices.push_back(create_vertex(points[f->points[0]]->pos, norm, texcoords, colors, i, j, 0, aabb));
			    vertices.push_back(create_vertex(points[f->points[k]]->pos, norm, texcoords, colors, i, j, k, aabb));
			    vertices.push_back(create_vertex(points[f->points[k + 1]]->pos, norm, texcoords, colors, i, j, k + 1, aabb));
		    }
            for (size_t k = 0, l = f->points.size(); k < l; ++k) {
			    border_vertices.push_back(create_vertex(points[f->points[k]]->pos, norm, texcoords, colors, i, j, k, aabb));
		    }
		    for (int k = 0, n = f->points.size() - 1; k < n; ++k) {
			    border_indices.push_back(start_idx + k);
			    border_indices.push_back(start_idx + k + 1);
		    }
		    border_indices.push_back(static_cast<unsigned short>(start_idx + f->points.size() - 1));
		    border_indices.push_back(start_idx);
		    start_idx += f->points.size();
	    }
    }
    if (!vertices.empty()) {
        FlushVertices(type, mesh, border_mesh, vertices, border_vertices, border_indices, *model);
    }
	model->aabb = aabb;

	return model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(VertexType type, const model::BrushModel& brush_model,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords,
                                const std::vector<std::vector<std::vector<sm::vec3>>>& colors)
{
    auto& src_brushes = brush_model.GetBrushes();
    std::vector<pm3::PolytopePtr> brushes;
    brushes.reserve(src_brushes.size());
    for (auto& b : src_brushes) {
        brushes.push_back(b.impl);
    }
    return PolymeshFromBrush(type, brushes, texcoords, colors);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromPolygon(const std::vector<sm::vec3>& polygon)
{
    auto brush_model = model::BrushBuilder::BrushFromPolygon(polygon);
    std::vector<sm::vec2> texcoords(polygon.size(), sm::vec2(0, 0));
    std::vector<sm::vec3> colors(polygon.size(), sm::vec3(1, 1, 1));
    auto model = model::BrushBuilder::PolymeshFromBrush(VertexType::PosNorm, *brush_model, {{ texcoords }}, {{ colors }});
    model->ext = std::move(brush_model);

    return model;
}

void BrushBuilder::UpdateVBO(Model& model, const BrushModel::Brush& brush)
{
    if (!brush.impl) {
        return;
    }

    static const sm::vec3 WHITE(1, 1, 1);

	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
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
				std::vector<sm::vec3> border;
				assert(f->points.size() > 2);
				for (size_t i = 1; i < f->points.size() - 1; ++i)
				{
					vertices.push_back(CreateVertex(f, points[f->points[0]]->pos, tex_w, tex_h, WHITE, aabb));
					vertices.push_back(CreateVertex(f, points[f->points[i]]->pos, tex_w, tex_h, WHITE, aabb));
					vertices.push_back(CreateVertex(f, points[f->points[i + 1]]->pos, tex_w, tex_h, WHITE, aabb));
				}
				for (auto& vert : f->points) {
					border_vertices.push_back(CreateVertex(f, points[vert]->pos, tex_w, tex_h, WHITE, aabb));
				}
			}
		}

        std::vector<float> vertex_buf, border_vertex_buf;
        dump_vert_buf(VertexType::PosNormTex, vertices, vertex_buf);
        dump_vert_buf(VertexType::PosNormTex, border_vertices, border_vertex_buf);

		// upload buffer data
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.meshes[i]->geometry.vbo, vertex_buf.data(),
			sizeof(float) * vertex_buf.size());
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.border_meshes[i]->geometry.vbo, border_vertex_buf.data(),
			sizeof(float) * border_vertex_buf.size());

	}
}

void BrushBuilder::UpdateVBO(Model& model, const model::BrushModel& brush_model)
{
    for (auto& brush : brush_model.GetBrushes()) {
        UpdateVBO(model, brush);
    }
}

void BrushBuilder::CreateMeshRenderBuf(VertexType type, model::Model::Mesh& mesh,
                                       const std::vector<Vertex>& vertices)
{
    ur::RenderContext::VertexInfo vi;

    std::vector<float> buf;
    dump_vert_buf(type, vertices, buf);

    vi.vn       = vertices.size();
    vi.vertices = buf.data();
    vi.stride   = calc_strid(type);

    vi.in = 0;
    vi.indices = nullptr;

    setup_vert_attr_list(type, vi);

    ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
        vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(false, vi.vn, 0));
    setup_geo_vert_type(type, mesh.geometry.vertex_type);
}

void BrushBuilder::CreateBorderMeshRenderBuf(VertexType type, model::Model::Mesh& mesh,
                                             const std::vector<Vertex>& vertices,
                                             const std::vector<unsigned short>& indices)
{
    ur::RenderContext::VertexInfo vi;

    std::vector<float> buf;
    dump_vert_buf(type, vertices, buf);

    vi.vn = vertices.size();
    vi.vertices = buf.data();
    vi.stride = calc_strid(type);

    vi.in = indices.size();
    vi.indices = &indices[0];

    setup_vert_attr_list(type, vi);

    ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
        vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(true, vi.in, 0));
    setup_geo_vert_type(type, mesh.geometry.vertex_type);
}

BrushBuilder::Vertex
BrushBuilder::CreateVertex(const pm3::FacePtr& face, const sm::vec3& pos, int tex_w, int tex_h,
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

void BrushBuilder::FlushVertices(VertexType type,
                                 std::unique_ptr<model::Model::Mesh>& mesh,
                                 std::unique_ptr<model::Model::Mesh>& border_mesh,
                                 std::vector<Vertex>& vertices,
                                 std::vector<Vertex>& border_vertices,
                                 std::vector<unsigned short>& border_indices,
                                 model::Model& dst)
{
    CreateMeshRenderBuf(type, *mesh, vertices);
    dst.meshes.push_back(std::move(mesh));
    vertices.clear();

    CreateBorderMeshRenderBuf(type, *border_mesh, border_vertices, border_indices);
    dst.border_meshes.push_back(std::move(border_mesh));
    border_vertices.clear();
}

}