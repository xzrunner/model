#include "model/BrushBuilder.h"
#include "model/typedef.h"

#include <polymesh3/Brush.h>
#include <unirender/RenderContext.h>
#include <unirender/Blackboard.h>

namespace model
{

const float BrushBuilder::VERTEX_SCALE = 0.01f;

std::unique_ptr<BrushModel>
BrushBuilder::BrushFromPolygon(const std::vector<sm::vec3>& polygon)
{
	if (polygon.size() < 3) {
		return nullptr;
	}

	// scale
	auto scaled_poly = polygon;
	const float scale = 1.0f / VERTEX_SCALE;
	for (auto& v : scaled_poly) {
		v *= scale;
	}

	// fix dir
	// should be clockwise, as left-hand system, top face's normal direction is y
	auto v0 = polygon[1] - polygon[0];
	auto v1 = polygon[2] - polygon[1];
	if (v0.Cross(v1).Dot({ 0, 1, 0 }) < 0) {
		std::reverse(scaled_poly.begin(), scaled_poly.end());
	}

	const float dy = 0.1f;
//	const float dy = 30;

	// brush data
    BrushModel::Brush brush;
    brush.desc.mesh_begin = 0;
    brush.desc.mesh_end   = 1;
	int face_num = scaled_poly.size() + 2;
    brush.desc.meshes.push_back({ 0, 0, 0, face_num });

    auto& faces = brush.impl->faces;
	faces.reserve(face_num);
	// top
	{
		sm::vec3 tri[3];
		tri[0] = scaled_poly[0];
		tri[1] = scaled_poly[1];
		tri[2] = scaled_poly[2];
		for (int i = 0; i < 3; ++i) {
			tri[i].y += dy;
		}
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(tri[0], tri[1], tri[2]);
		faces.push_back(face);
	}
	// bottom
	{
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(scaled_poly[2], scaled_poly[1], scaled_poly[0]);
		faces.push_back(face);
	}
	// edge faces
	for (size_t i = 0, n = scaled_poly.size(); i < n; ++i)
	{
		auto& v0 = scaled_poly[i];
		auto& v1 = scaled_poly[(i + 1) % scaled_poly.size()];
		auto face = std::make_shared<pm3::BrushFace>();
		face->plane = sm::Plane(v0, v1, { v1.x, v1.y + dy, v1.z });
		faces.push_back(face);
	}

    brush.impl->BuildVertices();
    brush.impl->BuildGeometry();

    auto model_model = std::make_unique<BrushModel>();
    std::vector<BrushModel::Brush> brushes;
    brushes.push_back(brush);
    model_model->SetBrushes(brushes);

    return model_model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(const std::vector<std::shared_ptr<pm3::Brush>>& brushes)
{
    auto model = std::make_unique<Model>();

	sm::cube aabb;
	int start_idx = 0;
    for (auto& b : brushes)
    {
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

	    for (auto& f : b->faces)
	    {
            auto& norm = f->plane.normal;
		    for (size_t i = 1; i < f->vertices.size() - 1; ++i)
		    {
			    vertices.push_back(CreateVertex(b->vertices[f->vertices[0]], norm, aabb));
			    vertices.push_back(CreateVertex(b->vertices[f->vertices[i]], norm, aabb));
			    vertices.push_back(CreateVertex(b->vertices[f->vertices[i + 1]], norm, aabb));
		    }
		    for (auto& vert : f->vertices) {
			    border_vertices.push_back(CreateVertex(b->vertices[vert], norm, aabb));
		    }
		    for (int i = 0, n = f->vertices.size() - 1; i < n; ++i) {
			    border_indices.push_back(start_idx + i);
			    border_indices.push_back(start_idx + i + 1);
		    }
		    border_indices.push_back(static_cast<unsigned short>(start_idx + f->vertices.size() - 1));
		    border_indices.push_back(start_idx);
		    start_idx += f->vertices.size();
	    }
	    if (!vertices.empty()) {
		    FlushVertices(mesh, border_mesh, vertices, border_vertices, border_indices, *model);
	    }
    }
	model->aabb = aabb;

	return model;
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromBrush(const model::BrushModel& brush_model)
{
    auto& src_brushes = brush_model.GetBrushes();
    std::vector<std::shared_ptr<pm3::Brush>> brushes;
    brushes.reserve(src_brushes.size());
    for (auto& b : src_brushes) {
        brushes.push_back(b.impl);
    }
    return PolymeshFromBrush(brushes);
}

std::unique_ptr<Model>
BrushBuilder::PolymeshFromPolygon(const std::vector<sm::vec3>& polygon)
{
    auto brush_model = model::BrushBuilder::BrushFromPolygon(polygon);
    auto model = model::BrushBuilder::PolymeshFromBrush(*brush_model);
    model->ext = std::move(brush_model);

    return model;
}

void BrushBuilder::UpdateVBO(Model& model, const BrushModel::Brush& brush)
{
    if (!brush.impl) {
        return;
    }

	auto& rc = ur::Blackboard::Instance()->GetRenderContext();
	auto& faces = brush.impl->faces;
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
				assert(f->vertices.size() > 2);
				for (size_t i = 1; i < f->vertices.size() - 1; ++i)
				{
					vertices.push_back(CreateVertex(f, brush.impl->vertices[f->vertices[0]], tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, brush.impl->vertices[f->vertices[i]], tex_w, tex_h, aabb));
					vertices.push_back(CreateVertex(f, brush.impl->vertices[f->vertices[i + 1]], tex_w, tex_h, aabb));
				}
				for (auto& vert : f->vertices) {
					border_vertices.push_back(CreateVertex(f, brush.impl->vertices[vert], tex_w, tex_h, aabb));
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
BrushBuilder::CreateVertex(const pm3::BrushFacePtr& face, const sm::vec3& pos, int tex_w, int tex_h, sm::cube& aabb)
{
    Vertex v;

    v.pos = pos * model::BrushBuilder::VERTEX_SCALE;
    aabb.Combine(v.pos);

    v.normal = face->plane.normal;

    if (tex_w == 0 || tex_h == 0) {
        v.texcoord.Set(0, 0);
    } else {
        v.texcoord = face->CalcTexCoords(
            pos, static_cast<float>(tex_w), static_cast<float>(tex_h));
    }

    return v;
}

BrushBuilder::Vertex
BrushBuilder::CreateVertex(const sm::vec3& pos, const sm::vec3& normal, sm::cube& aabb)
{
    Vertex v;

    v.pos = pos * model::BrushBuilder::VERTEX_SCALE;
    aabb.Combine(v.pos);

    v.normal = normal.Normalized();

    v.texcoord.Set(0, 0);

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