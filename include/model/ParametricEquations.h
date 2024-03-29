#pragma once

#include "model/ParametricSurface.h"

#include <SM_Vector.h>
#include <sm_const.h>

namespace model
{

class Box : public ParametricSurface
{
public:
    Box() {}
//    Box(float width, float height, float depth);

    virtual int  GetVertexCount() const override;
    virtual int  GetTriangleIndexCount() const override;

    virtual void GenerateVertices(int vertex_type, std::vector<float>& vertices) const override;
    virtual void GenerateTriangleIndices(std::vector<unsigned short>& indices) const override;

    static const char* const TYPE_NAME;

private:
    virtual const char* Type() const override { return TYPE_NAME; }

    virtual sm::vec3 Evaluate(const sm::vec2& domain) const override { return sm::vec3(); }

//private:
//    float m_width;
//    float m_height;
//    float m_depth;

}; // Box

class Cone : public ParametricSurface
{
public:
	Cone(float height, float radius);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_height;
    float m_radius;

}; // Cone

class Sphere : public ParametricSurface
{
public:
	Sphere(float radius, const sm::ivec2& divisions = sm::ivec2(20, 20));

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	float GetRadius() const { return m_radius; }

	static const char* const TYPE_NAME;

private:
    float m_radius;

}; // Sphere

class Ellipsoid : public ParametricSurface
{
public:
	Ellipsoid(const sm::vec3& radius);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	auto& GetRadius() const { return m_radius; }

	static const char* const TYPE_NAME;

private:
	sm::vec3 m_radius;

}; // Ellipsoid

class Torus : public ParametricSurface
{
public:
	Torus(float majorRadius, float minorRadius);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_majorRadius;
    float m_minorRadius;

}; // Torus

class TrefoilKnot : public ParametricSurface
{
public:
	TrefoilKnot(float scale);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_scale;

}; // TrefoilKnot

class MobiusStrip : public ParametricSurface
{
public:
	MobiusStrip(float scale);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_scale;

}; // MobiusStrip

class KleinBottle : public ParametricSurface
{
public:
	KleinBottle(float scale);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;
	virtual bool InvertNormal(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_scale;

}; // KleinBottle

}
