#include "model/GlobalClock.h"

namespace model
{

GlobalClock* GlobalClock::m_instance = nullptr;

GlobalClock* GlobalClock::Instance() 
{ 
	if (!m_instance) { 
		m_instance = new (GlobalClock)(); 
	} 
	return m_instance; 
}

GlobalClock::GlobalClock()
	: last_time(0)
{
}

void GlobalClock::Update(float dt)
{
	last_time += dt;
}

}