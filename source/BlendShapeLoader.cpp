#include "model/BlendShapeLoader.h"
#include "model/Model.h"

namespace model
{

void BlendShapeLoader::Load(Model& model, std::vector<std::unique_ptr<BlendShapeLoader::MeshData>>& meshes)
{
    for (auto& bs_mesh : meshes)
    {
        std::map<sm::vec3, int, sm::Vector3Cmp> lut;
        for (int i = 0, n = bs_mesh->vertices.size(); i < n; ++i) {
            auto stat = lut.insert({ bs_mesh->vertices[i], i });
 //           assert(stat.second);
        }

        for (auto& m : model.meshes)
        {
            if (m->name != bs_mesh->name) {
                continue;
            }

            auto& geo = m->geometry;
            const size_t n_bs = bs_mesh->blendshapes.size();
            geo.blendshape_data.reserve(n_bs);
            for (size_t i = 0; i < n_bs; ++i)
            {
                auto& src = bs_mesh->blendshapes[i];
                auto dst = std::make_unique<BlendShapeData>();
                dst->name = bs_mesh->blendshapes[i]->name;
                dst->vertices.resize(geo.ori_verts.size());
                geo.blendshape_data.emplace_back(std::move(dst));
            }
            for (size_t i = 0, n = geo.ori_verts.size(); i < n; ++i)
            {
                auto find = lut.find(geo.ori_verts[i]);
                assert(find != lut.end());
                int idx = find->second;
                for (size_t j = 0; j < n_bs; ++j) {
                    geo.blendshape_data[j]->vertices[i] = bs_mesh->blendshapes[j]->vertices[idx];
                }
            }

            // stat
            for (size_t i = 0, n = geo.blendshape_data.size(); i < n; ++i) {
                int count = 0;
                for (int j = 0; j < geo.n_vert; ++j) {
                    if (geo.ori_verts[j] == geo.blendshape_data[i]->vertices[j]) {
                        ++count;
                    }
                }
//                printf("same %f [%d/%d]\n", (float)count / geo.n_vert, count, geo.n_vert);
            }
        }
    }
}

}