#pragma once

#include "Model.h"

#include <SM_Vector.h>

#include <map>

namespace model 
{

class ModelObj : public Model
{
public:
	ModelObj();
//	ModelObj(const std::string& filepath, float scale = 1.0f);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual bool StoreToJson(rapidjson::Value& val,
		rapidjson::MemoryPoolAllocator<>& alloc) const override;
	virtual void LoadFromJson(const rapidjson::Value& val) override;

	static const char* const TYPE_NAME;

private:
	void Import(const std::string& filepath);
	void InitNormals();
	void InitAllMeshes();

	void HandleMaterial(const std::string& filepath);
	void HandleObject();
	void HandleMesh();
	void HandleVertice(const std::vector<std::string>& tokens);
	void HandleTextureCoordinate(const std::vector<std::string>& tokens);
	void HandleNormal(const std::vector<std::string>& tokens);
	void HandleFace(const std::vector<std::string>& tokens);

	void Error(std::ifstream& fin, const std::string& filepath);

	static void SplitString(const std::string& src, std::vector<std::string>& dst);
	static void SplitString(const std::string& src, std::vector<std::string>& dst, char delim);

	static sm::vec3 String2Vec3(const std::string& sx, const std::string& sy, const std::string& sz);

private:
	struct MaterialInfo
	{
		std::string name;
		sm::vec3 ambient;
		sm::vec3 diffuse;
		sm::vec3 specular;

		void clear() {
			name = "";
			ambient = diffuse = specular = sm::vec3(0, 0, 0);
		}
	};

	struct MeshInfo
	{
		int material_id;
		bool has_texture;
		bool has_normal;

		std::map<int, int> map_index;
		std::vector<sm::vec3> vertices;
		std::vector<sm::vec3> normals;
		std::vector<sm::ivec3> faces;

		void clear() {
			map_index.clear();
			vertices.clear();
			faces.clear();
		}

		int GetVertexCount() const;
	 	int GetTriangleIndexCount() const;
		void GenerateVertices(std::vector<float>& vertices, unsigned char flags) const;
	 	void GenerateTriangleIndices(std::vector<unsigned short>& indices) const;

		void AddFace(ModelObj& obj, int i0, int i1, int i2);
		int AddNode(ModelObj& obj, int index);
	};

	struct ObjectInfo
	{
		std::string name;
		std::vector<MeshInfo> meshes;

		void clear() {
			meshes.clear();
		}
	};

private:
	float m_scale;

	std::vector<sm::vec3>  m_vertices;
	std::vector<sm::ivec3> m_faces;
	std::vector<sm::vec3>  m_normals;
//	std::vector<uint16> indices;

	std::vector<MaterialInfo> m_materials;
	std::vector<ObjectInfo>   m_objects;

	ObjectInfo m_curr_object;
	MeshInfo   m_curr_mesh;

}; // ModelObj

}
