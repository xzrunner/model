#include "model/BrushBuilder.h"
#include "model/typedef.h"

#include <polymesh3/Geometry.h>
#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

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
BrushBuilder::PolymeshFromBrush(const std::vector<pm3::PolytopePtr>& brushes,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
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
			    vertices.push_back(CreateVertex(points[f->points[0]]->pos, norm, texcoords[i][j][0], aabb));
			    vertices.push_back(CreateVertex(points[f->points[k]]->pos, norm, texcoords[i][j][k], aabb));
			    vertices.push_back(CreateVertex(points[f->points[k + 1]]->pos, norm, texcoords[i][j][k + 1], aabb));
		    }
            for (size_t k = 0, l = f->points.size(); k < l; ++k) {
			    border_vertices.push_back(CreateVertex(points[f->points[k]]->pos, norm, texcoords[i][j][k], aabb));
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
        FlushVertices(mesh, border_mesh, vertices, border_vertices, border_indices, *model);
    }
	model->aabb = aabb;

	return model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(const model::BrushModel& brush_model,
                                const std::vector<std::vector<std::vector<sm::vec2>>>& texcoords)
{
    auto& src_brushes = brush_model.GetBrushes();
    std::vector<pm3::PolytopePtr> brushes;
    brushes.reserve(src_brushes.size());
    for (auto& b : src_brushes) {
        brushes.push_back(b.impl);
    }
    return PolymeshFromBrush(brushes, texcoords);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromPolygon(const std::vector<sm::vec3>& polygon)
{
    auto brush_model = model::BrushBuilder::BrushFromPolygon(polygon);
    std::vector<sm::vec2> texcoords(polygon.size(), sm::vec2(0, 0));
    auto model = model::BrushBuilder::PolymeshFromBrush(*brush_model, {{ texcoords }});
    model->ext = std::move(brush_model);

    return model;
}

void BrushBuilder::UpdateVBO(Model& model, const BrushModel::Brush& brush)
{
    if (!brush.impl) {
        return;
    }

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
					vertices.push_back(CreateVertex(f, points[f->points[0]]->pos, tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, points[f->points[i]]->pos, tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, points[f->points[i + 1]]->pos, tex_w, tex_h, aabb));
				}
				for (auto& vert : f->points) {
					border_vertices.push_back(CreateVertex(f, points[vert]->pos, tex_w, tex_h, aabb));
				}
			}
		}

		// upload buffer data
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.meshes[i]->geometry.vbo, vertices.data(),
			sizeof(Vertex) * vertices.size());
		rc.UpdateBufferRaw(ur::BUFFER_VERTEX, model.border_meshes[i]->geometry.vbo, border_vertices.data(),
			sizeof(Vertex) * border_vertices.size());

	}
}

void BrushBuilder::UpdateVBO(Model& model, const model::BrushModel& brush_model)
{
    for (auto& brush : brush_model.GetBrushes()) {
        UpdateVBO(model, brush);
    }
}

void BrushBuilder::CreateMeshRenderBuf(model::Model::Mesh& mesh,
                                       const std::vector<Vertex>& vertices)
{
    const int stride = sizeof(Vertex) / sizeof(float);

    ur::RenderContext::VertexInfo vi;

    vi.vn = vertices.size();
    vi.vertices = &vertices[0].pos.x;
    vi.stride = sizeof(Vertex);

    vi.in = 0;
    vi.indices = nullptr;

	vi.va_list.push_back(ur::VertexAttrib("pos",          3, 4, 32, 0));   // pos
	vi.va_list.push_back(ur::VertexAttrib("normal",       3, 4, 32, 12));  // normal
	vi.va_list.push_back(ur::VertexAttrib("texcoord",     2, 4, 32, 24));  // texcoord

    ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
        vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(false, vi.vn, 0));
    mesh.geometry.vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
}

void BrushBuilder::CreateBorderMeshRenderBuf(model::Model::Mesh& mesh,
                                             const std::vector<Vertex>& vertices,
                                             const std::vector<unsigned short>& indices)
{
    const int stride = sizeof(Vertex) / sizeof(float);

    ur::RenderContext::VertexInfo vi;

    vi.vn = vertices.size();
    vi.vertices = &vertices[0].pos.x;
    vi.stride = sizeof(Vertex);

    vi.in = indices.size();
    vi.indices = &indices[0];

	vi.va_list.push_back(ur::VertexAttrib("pos",          3, 4, 32, 0));   // pos
	vi.va_list.push_back(ur::VertexAttrib("normal",       3, 4, 32, 12));  // normal
	vi.va_list.push_back(ur::VertexAttrib("texcoord",     2, 4, 32, 24));  // texcoord

    ur::Blackboard::Instance()->GetRenderContext().CreateVAO(
        vi, mesh.geometry.vao, mesh.geometry.vbo, mesh.geometry.ebo);
    int idx = mesh.geometry.sub_geometries.size();
    mesh.geometry.sub_geometry_materials.push_back(idx);
    mesh.geometry.sub_geometries.push_back(model::SubmeshGeometry(true, vi.in, 0));
    mesh.geometry.vertex_type |= model::VERTEX_FLAG_TEXCOORDS;
}

BrushBuilder::Vertex
BrushBuilder::CreateVertex(const pm3::FacePtr& face, const sm::vec3& pos, int tex_w, int tex_h, sm::cube& aabb)
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

    return v;
}

BrushBuilder::Vertex
BrushBuilder::CreateVertex(const sm::vec3& pos, const sm::vec3& normal,
                           const sm::vec2& texcoord, sm::cube& aabb)
{
    Vertex v;

    v.pos = pos;
    aabb.Combine(v.pos);

    v.normal = normal.Normalized();

    v.texcoord = texcoord;

    return v;
}

void BrushBuilder::FlushVertices(std::unique_ptr<model::Model::Mesh>& mesh,
                                 std::unique_ptr<model::Model::Mesh>& border_mesh,
                                 std::vector<Vertex>& vertices,
                                 std::vector<Vertex>& border_vertices,
                                 std::vector<unsigned short>& border_indices,
                                 model::Model& dst)
{
    CreateMeshRenderBuf(*mesh, vertices);
    dst.meshes.push_back(std::move(mesh));
    vertices.clear();

    CreateBorderMeshRenderBuf(*border_mesh, border_vertices, border_indices);
    dst.border_meshes.push_back(std::move(border_mesh));
    border_vertices.clear();
}

}