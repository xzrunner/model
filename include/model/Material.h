#pragma once

#include <SM_Vector.h>

#include <boost/noncopyable.hpp>

namespace model
{

struct MaterialOld
{
	sm::vec3 ambient = { 0.04f, 0.04f, 0.04f };
	sm::vec3 diffuse = { 1, 1, 1 };
	sm::vec3 specular = { 1, 1, 1 };
	float    shininess = 50;
	void*    texture = nullptr;

	~MaterialOld();

}; // MaterialOld

struct Material
{

	sm::vec4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	sm::vec3 FresnelR0     = { 0.01f, 0.01f, 0.01f };
	float Roughness        = .25f;
	void* texture = nullptr;

}; // Material

}