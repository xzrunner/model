#pragma once

#include "model/ModelExtendType.h"

#include <boost/noncopyable.hpp>

namespace model
{

class ModelExtend : boost::noncopyable
{
public:
	virtual ~ModelExtend() {}

	virtual ModelExtendType Type() const = 0;

}; // ModelExtend

}