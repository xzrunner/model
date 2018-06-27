#pragma once

#include "model/ParametricSurface.h"

#include <SM_Vector.h>
#include <sm_const.h>

namespace model
{

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
	Sphere(float radius);

	virtual const char* Type() const override { return TYPE_NAME; }

	virtual sm::vec3 Evaluate(const sm::vec2& domain) const override;

	static const char* const TYPE_NAME;

private:
    float m_radius;

}; // Sphere

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
