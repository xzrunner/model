#pragma once

#include <memory>
#include <string>

namespace model
{

class Model;

class ModelFactory
{
public:
	static std::shared_ptr<Model> Create(const std::string& type);

}; // ModelFactory

}