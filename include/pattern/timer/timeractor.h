/**
 * @file timeractor.h
 * @brief Simplx timer actor
 * @copyright 2019 Scalewatch (www.scalewatch.com). All rights reserved.
 * Please see accompanying LICENSE file for licensing terms.
 */

#pragma once

#include "pattern/timer/timerevent.h"

namespace simplx
{

namespace timer
{

//---- Timer Actor -------------------------------------------------------------
    
class TimerActor: public Actor, private Actor::Callback
{
public:
	TimerActor();	// throws (std::bad_alloc, ShutdownException)
    
protected:

    inline
    void onCallback(const DateTime& utcTime) noexcept
    {   
        // protected for unit testing override [PL bs]
		eventHandler.onCallback(utcTime);
	}

private:

	friend class ::simplx::Actor;

	struct EventHandler
    {
		struct Client: ::simplx::MultiDoubleChainLink<Client>, Actor::Callback
        {
			TimerActor* timerActor;
			ActorId actorId;
			Time startTime;
			Time duration;
			bool inProgressFlag;

			inline
            Client() noexcept :
				timerActor(0), inProgressFlag(false)
            {
			}
			inline
            void pushTimeOut() noexcept
            {
				try
                {
					Event::Pipe(*timerActor, actorId).push<TimeOutEvent>(timerActor->eventHandler.currentUtcDateTime);
				} catch (std::bad_alloc&)
                {
					timerActor->registerCallback(*this);
				}
			}

			inline void onCallback() noexcept {
				pushTimeOut();
			}
		};

		typedef ::simplx::MultiDoubleChainLink<Client>::DoubleChain<> ClientChain;
		TimerActor& timerActor;
		Client client[MAX_NODE_COUNT];
		ClientChain inProgressClientChain;
		DateTime currentUtcDateTime;

		inline EventHandler(TimerActor& ptimerActor) noexcept :
				timerActor(ptimerActor)
        {
			for(int i = 0; i < MAX_NODE_COUNT; ++i)
            {
				client[i].timerActor = &timerActor;
			}
		}
		void onEvent(const GetEvent&);
		void onCallback(const DateTime&) noexcept;
	};

	EventHandler eventHandler;

	virtual void onCallback() noexcept; // override to use testing custom utc time, by enclosing call to protected onCallback(custom utc time).
};

using TimerService = TimerActor;

} // namespace

} // namespace

