/*
 * timer.cpp
 *
 * This tutorial shows how to use a timer by starting a timer service and deriving a TimerProxy.
 * It'll wait 3 times for 1 second and then destroy itself
 */

#include <iostream>
#include <iomanip>
#include <string>

#include "simplx.h"

#include "pattern/timer.h"

using namespace std;
using namespace simplx;

class MyActor: public Actor
{
	public:
	class timeOut1:public timer::TimerProxy
	{	public:
		Actor& actorRef;
		timeOut1(Actor& actor, int period):timer::TimerProxy(actor), actorRef(actor)
		{
			cout << "Setting timeout for adhoc at " << period << "\n";
			set(Time::Second(period));
		}
		void onTimeout(const simplx::DateTime&) noexcept
		{
			cout << "Received adhoc timer\n";
		}
	} timer1;

	class timeOut2:public timer::TimerProxy
	{
		private:
		int count;
		Actor& actorRef;
		public:
		timeOut2(Actor& actor, int period):timer::TimerProxy(actor), count(0), actorRef(actor)
		{
			cout << "Setting timeout for repeat at " << period << "\n";
			setRepeat(Time::Second(period));
		}
		void onTimeout(const simplx::DateTime& ) noexcept
		{
			cout << "Received periodic timer " << count << "\n";
			count++;
        
        	if (count == 15)   actorRef.requestDestroy();
		}
	} timer2;

public:
	MyActor()
        : timer1(static_cast<Actor&>(*this),31), timer2(static_cast<Actor&>(*this),3)
	{
	}
	
private:

	int m_Count;
};

//---- MAIN --------------------------------------------------------------------

int main()
{
	cout << "tutorial #10 : timer" << endl;
    
    Engine::StartSequence	startSequence;
	
    // add timer service on CPU core #0
	startSequence.addServiceActor<service::Timer, timer::TimerActor>(0);
    // add MyActor on CPU core #0
	startSequence.addActor<MyActor>(0);

	Engine	engine(startSequence);
	
    cout << "Press enter to exit..." << endl;
    cin.get();
	
	return 0;
}
