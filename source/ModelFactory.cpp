#include "model/ModelFactory.h"
#include "model/ModelObj.h"
#include "model/ModelParametric.h"

namespace model
{

std::shared_ptr<Model> ModelFactory::Create(const std::string& type)
{
	if (type == ModelObj::TYPE_NAME) {
//		return std::make_shared<ModelObj>();
		return nullptr;
	} else if (type == ModelParametric::TYPE_NAME) {
		return std::make_shared<ModelParametric>();
	} else {
		return nullptr;
	}
}

}