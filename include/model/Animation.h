#pragma once

#include "model/AnimType.h"

#include <boost/noncopyable.hpp>

namespace model
{

class Animation : boost::noncopyable
{
public:
	virtual ~Animation() {}

	virtual AnimType Type() const = 0;

}; // Animation

}