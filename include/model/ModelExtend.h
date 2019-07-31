#pragma once

#include "model/ModelExtendType.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace model
{

class ModelExtend : boost::noncopyable
{
public:
	virtual ~ModelExtend() {}

	virtual ModelExtendType Type() const = 0;

    virtual std::unique_ptr<ModelExtend> Clone() const = 0;

}; // ModelExtend

}