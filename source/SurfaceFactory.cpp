#include "model/SurfaceFactory.h"
#include "model/ParametricEquations.h"

namespace model
{

Surface* SurfaceFactory::Create(const std::string& name)
{
	Surface* surface = nullptr;
	if (name == Cone::TYPE_NAME) {
		surface = new Cone(2, 1);
	}  else if (name == Sphere::TYPE_NAME) {
		surface = new Sphere(1);
	} else if (name == Torus::TYPE_NAME) {
		surface = new Torus(0.5f, 0.2f);
	} else if (name == TrefoilKnot::TYPE_NAME) {
		surface = new TrefoilKnot(1);
	} else if (name == MobiusStrip::TYPE_NAME) {
		surface = new MobiusStrip(0.2f);
	} else if (name == KleinBottle::TYPE_NAME) {
		surface = new KleinBottle(0.1f);
	}
	return surface;
}

}