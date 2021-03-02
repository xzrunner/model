#pragma once

#include "model/ModelExtendType.h"

#include <unirender/noncopyable.h>

#include <memory>

namespace model
{

class ModelExtend : ur::noncopyable
{
public:
	virtual ~ModelExtend() {}

	virtual ModelExtendType Type() const = 0;

    virtual std::unique_ptr<ModelExtend> Clone() const = 0;

}; // ModelExtend

}