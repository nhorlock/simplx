/**
 * @file printer_actor_proxy_service.cpp
 * @author Neil Horlock (nhorlock@gmail.com)
 * @brief send to service via a singleton  "gateway"
 * 
 * This tutorial demonstrates how to start Actors and how they communicate.
 * It extends tutorial 4 to demonstrate the per-core-gateway routing model 
 * A ServiceActor is used to easily retrieve the PrinterActor from the service index.
 * The PrinterActor will be instantiated and will be waiting to receive PrintEvents.
 * Several WriterActors will be started and will send a PrintEvent to the PrinterActor.
 * They will send directly using a pipe and indirectly via a core singleton
 * The core singleton pattern is recommended for cross core communication to minimise
 * cache flushes.
 * @version 0.1
 * @date 2019-10-16
 * @copyright Copyright (c) 2019
 * 
 */
#include <iostream>

#include "simplx.h"

using namespace std;
using namespace simplx;


/*
 * Event that will be sent from the WriterActor to the PrinterActor.
 * The payload is a string that will be printed to the console.
 */
struct PrintEvent : Actor::Event
{
	PrintEvent(const string& message)
        : message(message)
    {
	}
    
	const string message;
};

// when PrinterActor receives PrintEvents it displays them to the console
class PrinterActor : public Actor
{
public:

	// service tag (or key) used to uniquely identify service
	struct ServiceTag : Service {};

    // ctor
	PrinterActor()
    {
		cout << "PrinterActor::PrinterActor()" << endl;
		registerEventHandler<PrintEvent>(*this);
    }
    
	// called when PrintEvent is received
	void onEvent(const PrintEvent& event)
    {
		cout << "PrinterActor::onEvent(): " << event.message << ", from " << event.getSourceActorId() << endl;
	}
};

/**
 * @brief Proxy class used by the write to grab/create a reference to the core singleton
 * calls to the print method are sent (function call) to the singleton, thus (in theory)
 * avoiding the cachge pollution of multiple cross core calls.
 */
class PrinterProxy 
{
public:
	struct NoServiceException: std::exception
    {
		virtual const char* what() const noexcept
        {
			return "PrinterProxy::NoServiceException";
		}
	};

    // ctor
	PrinterProxy(Actor& actor):gateway(actor.newReferencedSingletonActor<singletonServiceGateway<PrinterActor::ServiceTag>>())
    {
		cout << "PrinterProxy::PrinterProxy()\n";
    }
    
    void print(string& str)
    {
        gateway->print(str);
    }
    private:
    template<typename SvcTag>
    class singletonServiceGateway:public Actor ,public Actor::Callback
    {
        private:
            const Actor::ActorId& serviceId;
            Actor::Event::Pipe servicePipe;
            std::vector<string> printQueue;
        public:
            singletonServiceGateway():
                serviceId(this->getEngine().getServiceIndex().getServiceActorId<SvcTag>()),
                servicePipe(*this, serviceId)
            {
                if(serviceId == simplx::null)
                {
                    throw NoServiceException();
                }
                registerEventHandler<PrintEvent>(*this);
            }
            void print(string& str)
            {
                //servicePipe.push<PrintEvent>(str);
                printQueue.push_back(str);
                registerCallback(*this);
            }

            void onCallback()
            {
                for( auto& str : printQueue)
                {
                    servicePipe.push<::PrintEvent>(str);
                }
            }
        	// called when PrintEvent is received
            void onEvent(const PrintEvent& event)
            {
                // cout << "PrinterProxy::onEvent(): " << event.message << ", from " << event.getSourceActorId() << "\n";
                servicePipe.push<::PrintEvent>(event.message);
            }
    };
    protected:
    Actor::ActorReference<singletonServiceGateway<PrinterActor::ServiceTag>> gateway;
};


// WriterActor sends a PrintEvent containing a string to PrinterActor
class WriterActor : public Actor, public PrinterProxy
{
public:
    // ctor
    WriterActor(string msg):PrinterProxy(static_cast<Actor&>(*this))
    {
		cout << "WriterActor::CTOR()" << endl;
        
        // retrieve PrinterActor's id from the ServiceIndex
		const ActorId&   printerActorId = getEngine().getServiceIndex().getServiceActorId<PrinterActor::ServiceTag>();
        
		Event::Pipe pipe(*this, printerActorId);	// create uni-directional communication channel between WriterActor (this) and PrinterActor (printerActorId)
		pipe.push<PrintEvent>(msg+"(direct)");		// send PrintEvent through pipe
        Event::Pipe pipe2proxy(*this, gateway->getActorId());
		pipe2proxy.push<PrintEvent>(msg+"(indirect)");		// send PrintEvent through pipe
        print(msg);
	}
};

//---- MAIN --------------------------------------------------------------------

int main()
{
    cout << "tutorial #4 : printer actor service" << endl;
    
    Engine::StartSequence   startSequence;	        // configure initial Actor system
    
    startSequence.addServiceActor<PrinterActor::ServiceTag, PrinterActor>(0/*CPU core*/);
    startSequence.addActor<WriterActor>(1/*CPU core*/,string("hello A"));
    startSequence.addActor<WriterActor>(1/*CPU core*/, string("hello B"));
    startSequence.addActor<WriterActor>(2/*CPU core*/, string("hello C"));
    startSequence.addActor<WriterActor>(0/*CPU core*/, string("hello D"));

    Engine engine(startSequence);	                // start above actors
    sleep(10);
    cout << "Press enter to exit...";
    cin.get();

    return 0;
}
