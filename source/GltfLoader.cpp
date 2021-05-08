#include "model/GltfLoader.h"
#include "model/typedef.h"

#include <unirender/Device.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/IndexBuffer.h>
#include <unirender/TextureDescription.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace model
{

bool GltfLoader::Load(const ur::Device& dev, Model& model, const std::string& filepath)
{
	tinygltf::Model t_model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = loader.LoadASCIIFromFile(&t_model, &err, &warn, filepath);
	//bool ret = loader.LoadBinaryFromFile(&t_model, &err, &warn, filepath); // for binary glTF(.glb)

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return false;
	}

	LoadTextures(dev, model, t_model);

	LoadMaterials(dev, model, t_model);

	sm::cube aabb;
	LoadMeshes(dev, model, t_model, aabb);

	LoadNodes(dev, model, t_model);

	return true;
}

void GltfLoader::LoadTextures(const ur::Device& dev, Model& dst, const tinygltf::Model& src)
{
	for (auto& img : src.images)
	{
		ur::TextureFormat tf;
		if (img.component == 4 && img.bits == 8 && img.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			tf = ur::TextureFormat::RGBA8;
		}
		else 
		{
			assert(0);
		}

		ur::TextureDescription desc;
		desc.target = ur::TextureTarget::Texture2D;
		desc.width = img.width;
		desc.height = img.height;
		desc.format = tf;
		auto tex = dev.CreateTexture(desc, img.image.data());
		dst.textures.push_back({ img.uri, tex });
	}
}

void GltfLoader::LoadMaterials(const ur::Device& dev, Model& dst, const tinygltf::Model& src)
{
	for (auto& mtl : src.materials)
	{
		auto d_mtl = std::make_unique<Model::Material>();

		int albedo = src.textures[mtl.pbrMetallicRoughness.baseColorTexture.index].source;
		int roughness = src.textures[mtl.pbrMetallicRoughness.metallicRoughnessTexture.index].source;
		int emissive = src.textures[mtl.emissiveTexture.index].source;
		int oa = src.textures[mtl.occlusionTexture.index].source;
		int normal = src.textures[mtl.normalTexture.index].source;

		d_mtl->diffuse_tex = albedo;

		dst.materials.push_back(std::move(d_mtl));
	}
}

void GltfLoader::LoadMeshes(const ur::Device& dev, Model& dst, const tinygltf::Model& src, sm::cube& aabb)
{
	for (auto& mesh : src.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			int floats_per_vertex = 3;

			bool has_normal = prim.attributes.find("NORMAL") != prim.attributes.end();
			if (has_normal) {
				floats_per_vertex += 3;
			}

			bool has_texcoord = prim.attributes.find("TEXCOORD_0") != prim.attributes.end();
			if (has_texcoord) {
				floats_per_vertex += 2;
			}

			std::vector<sm::vec3> positions;
			{
				auto itr = prim.attributes.find("POSITION");
				assert(itr != prim.attributes.end());
				const tinygltf::Accessor& accessor = src.accessors[itr->second];
				const tinygltf::BufferView& buffer_view = src.bufferViews[accessor.bufferView];

				const tinygltf::Buffer& buffer = src.buffers[buffer_view.buffer];
				assert(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				const float* src_positions = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

				positions.resize(accessor.count);
				for (size_t i = 0; i < accessor.count; ++i) {
					for (int j = 0; j < 3; ++j) {
						positions[i].xyz[j] = src_positions[i * 3 + j];
					}
				}
			}

			std::vector<sm::vec3> normals;
			if (has_normal)
			{
				auto itr = prim.attributes.find("NORMAL");
				assert(itr != prim.attributes.end());
				const tinygltf::Accessor& accessor = src.accessors[itr->second];
				const tinygltf::BufferView& buffer_view = src.bufferViews[accessor.bufferView];

				const tinygltf::Buffer& buffer = src.buffers[buffer_view.buffer];
				assert(accessor.type == TINYGLTF_TYPE_VEC3 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				const float* src_normals = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

				normals.resize(accessor.count);
				for (size_t i = 0; i < accessor.count; ++i) {
					for (int j = 0; j < 3; ++j) {
						normals[i].xyz[j] = src_normals[i * 3 + j];
					}
				}

				assert(normals.size() == positions.size());
			}

			std::vector<sm::vec2> texcoords;
			if (has_texcoord)
			{
				auto itr = prim.attributes.find("TEXCOORD_0");
				assert(itr != prim.attributes.end());
				const tinygltf::Accessor& accessor = src.accessors[itr->second];
				const tinygltf::BufferView& buffer_view = src.bufferViews[accessor.bufferView];

				const tinygltf::Buffer& buffer = src.buffers[buffer_view.buffer];
				assert(accessor.type == TINYGLTF_TYPE_VEC2 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
				const float* src_texcoords = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

				texcoords.resize(accessor.count);
				for (size_t i = 0; i < accessor.count; ++i) {
					for (int j = 0; j < 2; ++j) {
						texcoords[i].xy[j] = src_texcoords[i * 2 + j];
					}
				}

				assert(texcoords.size() == positions.size());
			}

			uint8_t* buf = new uint8_t[positions.size() * floats_per_vertex * sizeof(float)];
			uint8_t* ptr = buf;
			for (int i = 0; i < positions.size(); ++i)
			{
				auto& p = positions[i];
				memcpy(ptr, &p.x, sizeof(float) * 3);
				ptr += sizeof(float) * 3;
				aabb.Combine(p);

				if (has_normal)
				{
					auto& nor = normals[i];
					memcpy(ptr, &nor.x, sizeof(float) * 3);
					ptr += sizeof(float) * 3;
				}
				if (has_texcoord)
				{
					auto& t = texcoords[i];
					float x = t.x;
					if (x > 1) {
						x -= std::floor(x);
					}
					memcpy(ptr, &x, sizeof(float));
					ptr += sizeof(float);
					float y = t.y;
					if (y > 1) {
						y -= std::floor(y);
					}
					y = 1.0f - y;
					memcpy(ptr, &y, sizeof(float));
					ptr += sizeof(float);
				}
			}

			auto mesh = std::make_unique<Model::Mesh>();

			auto va = dev.CreateVertexArray();

			// indices
			std::vector<unsigned short> indices;
			{
				const tinygltf::Accessor& accessor = src.accessors[prim.indices];
				const tinygltf::BufferView& buffer_view = src.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = src.buffers[buffer_view.buffer];

				const tinygltf::Buffer& idx_buffer = src.buffers[buffer_view.buffer];
				assert(accessor.type == TINYGLTF_TYPE_SCALAR && accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);
				const unsigned short* src_indices = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
				indices.resize(accessor.count);
				for (size_t i = 0; i < accessor.count; ++i) {
					indices[i] = src_indices[i];
				}
				auto ibuf_sz = sizeof(uint16_t) * indices.size();
				auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
				ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
				va->SetIndexBuffer(ibuf);

				auto vbuf_sz = sizeof(float) * floats_per_vertex * positions.size();
				auto vbuf = dev.CreateVertexBuffer(ur::BufferUsageHint::StaticDraw, vbuf_sz);
				vbuf->ReadFromMemory(buf, vbuf_sz, 0);
				va->SetVertexBuffer(vbuf);
			}

			std::vector<std::shared_ptr<ur::VertexInputAttribute>> vbuf_attrs;

			int stride = 0;
			// pos
			stride += 4 * 3;
			// normal
			if (has_normal) {
				stride += 4 * 3;
			}
			// texcoord
			if (has_texcoord) {
				stride += 4 * 2;
			}

			int attr_loc = 0;
			int offset = 0;
			// pos
			vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
				attr_loc++, ur::ComponentDataType::Float, 3, offset, stride));
			offset += 4 * 3;
			// normal
			if (has_normal)
			{
				mesh->geometry.vertex_type |= VERTEX_FLAG_NORMALS;
				vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
					attr_loc++, ur::ComponentDataType::Float, 3, offset, stride));
				offset += 4 * 3;
			}
			// texcoord
			if (has_texcoord)
			{
				mesh->geometry.vertex_type |= VERTEX_FLAG_TEXCOORDS;
				vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
					attr_loc++, ur::ComponentDataType::Float, 2, offset, stride));
				offset += 4 * 2;
			}

			va->SetVertexBufferAttrs(vbuf_attrs);

			mesh->geometry.vertex_array = va;
			int idx = mesh->geometry.sub_geometries.size();
			mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, indices.size(), 0));
			mesh->geometry.sub_geometry_materials.push_back(idx);

			delete[] buf;

			dst.meshes.push_back(std::move(mesh));
		}
	}
}

void GltfLoader::LoadNodes(const ur::Device& dev, Model& dst, const tinygltf::Model& src)
{
	for (auto& node : src.nodes)
	{
		auto d_node = std::make_unique<Model::Node>();
		d_node->mesh = node.mesh;
		if (!node.rotation.empty()) {
			d_node->mat = sm::mat4(sm::Quaternion(
				node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]
			));
		}
		dst.nodes.push_back(std::move(d_node));
	}
}

}