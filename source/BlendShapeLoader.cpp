#include "model/BlendShapeLoader.h"
#include "model/Model.h"

namespace model
{

void BlendShapeLoader::Load(Model& model, std::vector<std::unique_ptr<MeshData>>& meshes)
{
    for (auto& bs_mesh : meshes)
    {
        std::map<sm::vec3, int, sm::Vector3Cmp> lut;
        for (int i = 0, n = bs_mesh->mesh->verts.size(); i < n; ++i) {
            auto stat = lut.insert({ bs_mesh->mesh->verts[i], i });
 //           assert(stat.second);
        }

        for (auto& m : model.meshes)
        {
            if (m->name != bs_mesh->mesh->name) {
                continue;
            }

            auto& geo = m->geometry;
            const size_t n_bs = bs_mesh->blendshapes.size();
            geo.blendshape_data.reserve(n_bs);

            std::vector<std::vector<sm::vec3>> buf;
            buf.resize(n_bs);
            for (auto& b : buf) {
                b.resize(geo.n_vert);
            }

            for (size_t i = 0; i < n_bs; ++i)
            {
                auto& src = bs_mesh->blendshapes[i];
                auto dst = std::make_unique<BlendShapeData>();
                dst->name = bs_mesh->blendshapes[i]->name;
                geo.blendshape_data.emplace_back(std::move(dst));
            }

            std::vector<sm::vec3> ori_verts;
            ori_verts.resize(geo.n_vert);
            for (size_t i = 0, n = geo.n_vert; i < n; ++i) {
                ori_verts[i] = *(sm::vec3*)(geo.vert_buf + i * geo.vert_stride);
            }

            for (size_t i = 0, n = geo.n_vert; i < n; ++i)
            {
                auto find = lut.find(ori_verts[i]);
                assert(find != lut.end());
                int idx = find->second;
                for (size_t j = 0; j < n_bs; ++j) {
                    buf[j][i] = bs_mesh->blendshapes[j]->verts[idx];
                }
            }
            for (size_t i = 0; i < n_bs; ++i)
            {
                geo.blendshape_data[i]->SetVertices(ori_verts, buf[i]);
            }

            //// stat
            //for (size_t i = 0, n = geo.blendshape_data.size(); i < n; ++i) {
            //    int count = 0;
            //    for (int j = 0; j < geo.n_vert; ++j) {
            //        if (geo.ori_verts[j] == geo.blendshape_data[i]->vertices[j]) {
            //            ++count;
            //        }
            //    }
            //    printf("same %f [%d/%d]\n", (float)count / geo.n_vert, count, geo.n_vert);
            //}
        }
    }
}

}