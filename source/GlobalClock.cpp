#include "model/GlobalClock.h"

namespace model
{

CU_SINGLETON_DEFINITION(GlobalClock)

GlobalClock::GlobalClock()
	: last_time(0)
{
}

void GlobalClock::Update(float dt)
{
	last_time += dt;
}

}