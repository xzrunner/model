#pragma once

#include <cu/cu_macro.h>

namespace model
{

class GlobalClock
{
public:
	void Update(float dt);

	float GetTime() const { return last_time; }

private:
	float last_time;

	CU_SINGLETON_DECLARATION(GlobalClock)

}; // GlobalClock

}