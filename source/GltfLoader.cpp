#include "model/GltfLoader.h"
#include "model/typedef.h"
#include "model/Model.h"
#include "model/gltf/Model.h"

#include <unirender/Device.h>
#include <unirender/VertexBuffer.h>
#include <unirender/VertexArray.h>
#include <unirender/ComponentDataType.h>
#include <unirender/VertexInputAttribute.h>
#include <unirender/IndexBuffer.h>
#include <unirender/TextureDescription.h>
#include <unirender/TextureSampler.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

namespace
{

void load_texcoords(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<sm::vec2>& texcoords)
{
	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
	assert(accessor.type == TINYGLTF_TYPE_VEC2 && accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
	const float* src_texcoords = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

	texcoords.resize(accessor.count);
	for (size_t i = 0; i < accessor.count; ++i) {
		for (int j = 0; j < 2; ++j) {
			float v = src_texcoords[i * 2 + j];
			texcoords[i].xy[j] = v;
		}
	}
}

}

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

bool GltfLoader::Load(const ur::Device& dev, gltf::Model& model, const std::string& filepath)
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

	auto samplers = LoadSamplers(dev, t_model);
	auto textures = LoadTextures(dev, t_model, samplers);
	auto materials = LoadMaterials(dev, t_model, textures);
	auto meshes = LoadMeshes(dev, t_model, materials);
	auto nodes = LoadNodes(dev, t_model, meshes);
	auto scenes = LoadScenes(dev, t_model, nodes);

	model.scenes = scenes;
	model.scene = scenes[t_model.defaultScene];

	return true;
}

std::shared_ptr<ur::Texture> GltfLoader::LoadTexture(const ur::Device& dev, const tinygltf::Image& img)
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
	return dev.CreateTexture(desc, img.image.data());
}

std::shared_ptr<ur::VertexArray> 
GltfLoader::LoadVertexArray(const ur::Device& dev, const tinygltf::Model& model, const tinygltf::Primitive& prim, unsigned int& vertex_type)
{
	int floats_per_vertex = 3;

	bool has_normal = prim.attributes.find("NORMAL") != prim.attributes.end();
	if (has_normal) {
		floats_per_vertex += 3;
	}

	bool has_texcoord0 = prim.attributes.find("TEXCOORD_0") != prim.attributes.end();
	if (has_texcoord0) {
		floats_per_vertex += 2;
	}

	bool has_texcoord1 = prim.attributes.find("TEXCOORD_1") != prim.attributes.end();
	if (has_texcoord1) {
		floats_per_vertex += 2;
	}

	std::vector<sm::vec3> positions;
	{
		auto itr = prim.attributes.find("POSITION");
		assert(itr != prim.attributes.end());
		const tinygltf::Accessor& accessor = model.accessors[itr->second];
		const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

		const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
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
		const tinygltf::Accessor& accessor = model.accessors[itr->second];
		const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

		const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
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

	std::vector<sm::vec2> texcoords0;
	if (has_texcoord0)
	{
		auto itr = prim.attributes.find("TEXCOORD_0");
		assert(itr != prim.attributes.end());
		const tinygltf::Accessor& accessor = model.accessors[itr->second];
		load_texcoords(model, accessor, texcoords0);
		assert(texcoords0.size() == positions.size());
	}

	std::vector<sm::vec2> texcoords1;
	if (has_texcoord1)
	{
		auto itr = prim.attributes.find("TEXCOORD_1");
		assert(itr != prim.attributes.end());
		const tinygltf::Accessor& accessor = model.accessors[itr->second];
		load_texcoords(model, accessor, texcoords1);
		assert(texcoords1.size() == positions.size());
	}

	uint8_t* buf = new uint8_t[positions.size() * floats_per_vertex * sizeof(float)];
	uint8_t* ptr = buf;
	for (int i = 0; i < positions.size(); ++i)
	{
		auto& p = positions[i];
		memcpy(ptr, &p.x, sizeof(float) * 3);
		ptr += sizeof(float) * 3;
		//aabb.Combine(p);

		if (has_normal)
		{
			auto& nor = normals[i];
			memcpy(ptr, &nor.x, sizeof(float) * 3);
			ptr += sizeof(float) * 3;
		}
		if (has_texcoord0)
		{
			auto& tc = texcoords0[i];
			memcpy(ptr, &tc.x, sizeof(float) * 2);
			ptr += sizeof(float) * 2;
		}
		if (has_texcoord1)
		{
			auto& tc = texcoords1[i];
			memcpy(ptr, &tc.x, sizeof(float) * 2);
			ptr += sizeof(float) * 2;
		}
	}

	auto mesh = std::make_unique<Model::Mesh>();

	auto va = dev.CreateVertexArray();

	// indices
	int indices_num = 0;
	{
		const tinygltf::Accessor& accessor = model.accessors[prim.indices];
		const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

		const tinygltf::Buffer& idx_buffer = model.buffers[buffer_view.buffer];
		assert(accessor.type == TINYGLTF_TYPE_SCALAR);
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			std::vector<unsigned short> indices;
			const unsigned short* src_indices = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
			indices.resize(accessor.count);
			for (size_t i = 0; i < accessor.count; ++i) {
				indices[i] = src_indices[i];
			}
			auto ibuf_sz = sizeof(uint16_t) * indices.size();
			auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
			ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
			ibuf->SetDataType(ur::IndexBufferDataType::UnsignedShort);
			va->SetIndexBuffer(ibuf);
			indices_num = indices.size();
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			std::vector<unsigned int> indices;
			const unsigned int* src_indices = reinterpret_cast<const unsigned int*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
			indices.resize(accessor.count);
			for (size_t i = 0; i < accessor.count; ++i) {
				indices[i] = src_indices[i];
			}
			auto ibuf_sz = sizeof(uint32_t) * indices.size();
			auto ibuf = dev.CreateIndexBuffer(ur::BufferUsageHint::StaticDraw, ibuf_sz);
			ibuf->ReadFromMemory(indices.data(), ibuf_sz, 0);
			ibuf->SetDataType(ur::IndexBufferDataType::UnsignedInt);
			va->SetIndexBuffer(ibuf);
			indices_num = indices.size();
		}
		else
		{
			assert(0);
		}

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
	if (has_texcoord0) {
		stride += 4 * 2;
	}
	if (has_texcoord1) {
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
		vertex_type |= VERTEX_FLAG_NORMALS;
		vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
			attr_loc++, ur::ComponentDataType::Float, 3, offset, stride));
		offset += 4 * 3;
	}
	// texcoord
	if (has_texcoord0)
	{
		vertex_type |= VERTEX_FLAG_TEXCOORDS0;
		vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
			attr_loc++, ur::ComponentDataType::Float, 2, offset, stride));
		offset += 4 * 2;
	}
	if (has_texcoord1)
	{
		vertex_type |= VERTEX_FLAG_TEXCOORDS1;
		vbuf_attrs.push_back(std::make_shared<ur::VertexInputAttribute>(
			attr_loc++, ur::ComponentDataType::Float, 2, offset, stride));
		offset += 4 * 2;
	}

	va->SetVertexBufferAttrs(vbuf_attrs);

	va->GetIndexBuffer()->SetCount(indices_num);

	delete[] buf;

	return va;
}

void GltfLoader::LoadTextures(const ur::Device& dev, Model& dst, const tinygltf::Model& src)
{
	for (auto& img : src.images) {
		dst.textures.push_back({ img.uri, LoadTexture(dev, img) });
	}
}

void GltfLoader::LoadMaterials(const ur::Device& dev, Model& dst, const tinygltf::Model& src)
{
	auto get_img_idx = [](const tinygltf::Model& model, int tex_idx)->int
	{
		if (tex_idx < 0) {
			return -1;
		} else {
			return model.textures[tex_idx].source;
		}
	};

	for (auto& mtl : src.materials)
	{
		auto d_mtl = std::make_unique<Model::Material>();

		d_mtl->diffuse_tex   = get_img_idx(src, mtl.pbrMetallicRoughness.baseColorTexture.index);
		d_mtl->metallic_roughness_tex = get_img_idx(src, mtl.pbrMetallicRoughness.metallicRoughnessTexture.index);
		d_mtl->emissive_tex  = get_img_idx(src, mtl.emissiveTexture.index);
		d_mtl->occlusion_tex = get_img_idx(src, mtl.occlusionTexture.index);
		d_mtl->normal_tex    = get_img_idx(src, mtl.normalTexture.index);

		dst.materials.push_back(std::move(d_mtl));
	}
}

void GltfLoader::LoadMeshes(const ur::Device& dev, Model& dst, const tinygltf::Model& src, sm::cube& aabb)
{
	for (auto& mesh : src.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			unsigned int vertex_type = 0;
			auto va = LoadVertexArray(dev, src, prim, vertex_type);

			auto mesh = std::make_unique<Model::Mesh>();
			mesh->geometry.vertex_type = vertex_type;

			mesh->geometry.vertex_array = va;
			int idx = mesh->geometry.sub_geometries.size();
			mesh->geometry.sub_geometries.push_back(SubmeshGeometry(true, va->GetIndexBuffer()->GetCount(), 0));
			mesh->geometry.sub_geometry_materials.push_back(idx);

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

std::vector<std::shared_ptr<ur::TextureSampler>> 
GltfLoader::LoadSamplers(const ur::Device& dev, const tinygltf::Model& model)
{
	std::vector<std::shared_ptr<ur::TextureSampler>> ret;
	for (auto& src : model.samplers)
	{
		ur::TextureMinificationFilter min_filter;
		switch (src.minFilter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			min_filter = ur::TextureMinificationFilter::Nearest;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			min_filter = ur::TextureMinificationFilter::Linear;
			break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
			min_filter = ur::TextureMinificationFilter::NearestMipmapNearest;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
			min_filter = ur::TextureMinificationFilter::LinearMipmapNearest;
			break;
		case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
			min_filter = ur::TextureMinificationFilter::NearestMipmapLinear;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
			min_filter = ur::TextureMinificationFilter::LinearMipmapLinear;
			break;
		default:
			min_filter = ur::TextureMinificationFilter::Linear;
		}

		ur::TextureMagnificationFilter mag_filter;
		switch (src.magFilter)
		{
		case TINYGLTF_TEXTURE_FILTER_NEAREST:
			mag_filter = ur::TextureMagnificationFilter::Nearest;
			break;
		case TINYGLTF_TEXTURE_FILTER_LINEAR:
			mag_filter = ur::TextureMagnificationFilter::Linear;
			break;
		default:
			mag_filter = ur::TextureMagnificationFilter::Linear;
		}

		ur::TextureWrap wrap_s;
		switch (src.wrapS)
		{
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			wrap_s = ur::TextureWrap::ClampToEdge;
			break;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			wrap_s = ur::TextureWrap::MirroredRepeat;
			break;
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			wrap_s = ur::TextureWrap::Repeat;
			break;
		default:
			wrap_s = ur::TextureWrap::Repeat;
		}

		ur::TextureWrap wrap_t;
		switch (src.wrapT)
		{
		case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
			wrap_t = ur::TextureWrap::ClampToEdge;
			break;
		case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
			wrap_t = ur::TextureWrap::MirroredRepeat;
			break;
		case TINYGLTF_TEXTURE_WRAP_REPEAT:
			wrap_t = ur::TextureWrap::Repeat;
			break;
		default:
			wrap_t = ur::TextureWrap::Repeat;
		}

		ret.push_back(dev.CreateTextureSampler(min_filter, mag_filter, wrap_s, wrap_t));
	}
	return ret;
}

std::vector<std::shared_ptr<gltf::Texture>> 
GltfLoader::LoadTextures(const ur::Device& dev, const tinygltf::Model& model, const  std::vector<std::shared_ptr<ur::TextureSampler>>& samplers)
{
	std::vector<std::shared_ptr<ur::Texture>> images;
	for (auto& img : model.images) {
		images.push_back(LoadTexture(dev, img));
	}

	std::vector<std::shared_ptr<gltf::Texture>> ret;
	for (auto& src : model.textures)
	{
		auto dst = std::make_shared<gltf::Texture>();
		dst->image = images[src.source];
		dst->sampler = samplers[src.sampler];
		ret.push_back(dst);
	}
	return ret;
}

std::vector<std::shared_ptr<gltf::Material>> 
GltfLoader::LoadMaterials(const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Texture>>& textures)
{
	std::vector<std::shared_ptr<gltf::Material>> ret;
	for (auto& src : model.materials)
	{
		auto dst = std::make_shared<gltf::Material>();

		dst->name = src.name;

		if (src.emissiveTexture.index >= 0)
		{
			dst->emissive = std::make_shared<gltf::Material::Emissive>();

			dst->emissive->texture = textures[src.emissiveTexture.index];
			dst->emissive->tex_coord = src.emissiveTexture.texCoord;

			for (int i = 0; i < 3; ++i) {
				dst->emissive->factor.xyz[i] = src.emissiveFactor[i];
			}
		}

		if (src.normalTexture.index >= 0)
		{
			dst->normal = std::make_shared<gltf::Material::Normal>();

			dst->normal->texture = textures[src.normalTexture.index];
			dst->normal->tex_coord = src.normalTexture.texCoord;
		}

		if (src.occlusionTexture.index >= 0)
		{
			dst->occlusion = std::make_shared<gltf::Material::Occlusion>();

			dst->occlusion->texture = textures[src.occlusionTexture.index];
			dst->occlusion->tex_coord = src.occlusionTexture.texCoord;
		}

		if (src.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		{
			dst->metallic_roughness = std::make_shared<gltf::Material::MetallicRoughness>();

			dst->metallic_roughness->texture = textures[src.pbrMetallicRoughness.metallicRoughnessTexture.index];
			dst->metallic_roughness->tex_coord = src.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;
			
			dst->metallic_roughness->metallic_factor = src.pbrMetallicRoughness.metallicFactor;
			dst->metallic_roughness->roughness_factor = src.pbrMetallicRoughness.roughnessFactor;
		}

		if (src.pbrMetallicRoughness.baseColorTexture.index >= 0)
		{
			dst->base_color = std::make_shared<gltf::Material::BaseColor>();

			dst->base_color->texture = textures[src.pbrMetallicRoughness.baseColorTexture.index];
			dst->base_color->tex_coord = src.pbrMetallicRoughness.baseColorTexture.texCoord;

			for (int i = 0; i < 4; ++i) {
				dst->base_color->factor.xyzw[i] = src.pbrMetallicRoughness.baseColorFactor[i];
			}
		}

		ret.push_back(dst);
	}
	return ret;
}

std::vector<std::shared_ptr<gltf::Mesh>> 
GltfLoader::LoadMeshes(const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Material>>& materials)
{
	std::vector<std::shared_ptr<gltf::Mesh>> ret;
	for (auto& src_mesh : model.meshes)
	{
		auto dst_mesh = std::make_shared<gltf::Mesh>();
		dst_mesh->name = src_mesh.name;
		for (auto& src_prim : src_mesh.primitives)
		{
			auto dst_prim = std::make_shared<gltf::Primitive>();

			dst_prim->material = materials[src_prim.material];

			unsigned int vertex_type = 0;
			dst_prim->va = LoadVertexArray(dev, model, src_prim, vertex_type);

			dst_mesh->primitives.push_back(dst_prim);
		}
		ret.push_back(dst_mesh);
	}
	return ret;
}

std::vector<std::shared_ptr<gltf::Node>> 
GltfLoader::LoadNodes(const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Mesh>>& meshes)
{
	std::vector<std::shared_ptr<gltf::Node>> ret;
	for (auto& src : model.nodes)
	{
		auto dst = std::make_shared<gltf::Node>();

		dst->name = src.name;
		if (src.mesh >= 0) {
			dst->mesh = meshes[src.mesh];
		}
		if (!src.translation.empty()) {
			for (int i = 0; i < 3; ++i) {
				dst->translation.xyz[i] = src.translation[i];
			}
		}
		if (!src.rotation.empty()) {
			for (int i = 0; i < 4; ++i) {
				dst->rotation.xyzw[i] = src.rotation[i];
			}
		}
		if (!src.scale.empty()) {
			for (int i = 0; i < 3; ++i) {
				dst->scale.xyz[i] = src.scale[i];
			}
		}

		ret.push_back(dst);
	}
	return ret;
}

std::vector<std::shared_ptr<gltf::Scene>> 
GltfLoader::LoadScenes(const ur::Device& dev, const tinygltf::Model& model, const std::vector<std::shared_ptr<gltf::Node>>& nodes)
{
	std::vector<std::shared_ptr<gltf::Scene>> ret;
	for (auto& src : model.scenes)
	{
		auto dst = std::make_shared<gltf::Scene>();

		dst->name = src.name;

		for (auto& node : src.nodes) {
			dst->nodes.push_back(nodes[node]);
		}

		ret.push_back(dst);
	}
	return ret;
}

}