#pragma once

namespace model
{

class GlobalClock
{
public:
	void Update(float dt);

	float GetTime() const { return last_time; }

	static GlobalClock* Instance();

private:
	GlobalClock();
	~GlobalClock();
	
private:
	float last_time;

	static GlobalClock* m_instance;

}; // GlobalClock

}