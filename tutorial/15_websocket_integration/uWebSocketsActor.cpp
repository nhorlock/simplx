#include <iostream>
#include <iomanip>
#include <string>

#include "simplx_core/connector/uwebsocket/uWebSocketsPlugin.h"
#include "simplx.h"

using namespace tredzone;

struct PerSocketData {};
struct RequestMsgEvent : Actor::Event
{
	RequestMsgEvent(uWS::HttpResponse<false>*const target, std::string url, std::string method, std::string query)
		: m_target(target), m_url(url), m_method(method), m_query(query)
	{
	}
	uWS::HttpResponse<false>* const  m_target;
	std::string m_url; 
    std::string m_method; 
    std::string m_query;
};

struct ResponseMsgEvent : Actor::Event
{
	ResponseMsgEvent(uWS::HttpResponse<false>*const target, const string &data, uWS::OpCode opCode)
		: m_target(target), m_data(data), m_opCode(opCode)
	{
	}
	uWS::HttpResponse<false>* const  m_target;
	std::string	m_data;
    uWS::OpCode m_opCode;
};

//---- BackEnd Actor Service ------------------------------------------------------

class BackEndActor : public Actor
{
public:
    BackEndActor()
	{
    	std::cout << "BackEndActor::CTOR" << std::endl;
        registerEventHandler<RequestMsgEvent>(*this);
    }

    // upon receiving a Request (from Rest)
    void onEvent(const RequestMsgEvent& e)
	{
        Event::Pipe	pipe(*this, e.getSourceActorId());

    	std::cout << "Received request : " << std::endl 
        << "    url:    " << e.m_url << std::endl
        << "    method: " << e.m_method << std::endl
        << "    query:  " << e.m_query << std::endl;
		pipe.push<ResponseMsgEvent>(e.m_target, "request received and treated by backend", uWS::OpCode::TEXT);
    }
};

struct BackEndTag : public Service{};


//---- REST ACTOR --------------------------------------------------------------
class RestActor;
RestActor * rap;

class RestActor : public Actor, public Actor::Callback
{
private:
    class LazyInit : public Actor::Callback
    {
        RestActor * parent;
        public:
        explicit LazyInit(RestActor * p):parent{p}{};
        void onCallback(){
            std::cout << "This is the Init CB\n"; 
            parent->initWS();
            parent->registerCallback(*parent);
        }
    } lazyInit;
    uint m_i;
    bool m_initialized;
    struct uWS::TemplatedApp<false> * m_loop;
    Event::Pipe m_pipe;
public:
    RestActor() : lazyInit{this}, m_pipe{*this}
	{
    	std::cout << "RestActor::CTOR" << std::endl;
        registerEventHandler<ResponseMsgEvent>(*this);
        const ActorId	destinationActorId = getEngine().getServiceIndex().getServiceActorId<BackEndTag>();
        m_pipe.setDestinationActorId(destinationActorId);    
        // initWS();   
        auto nextCallback = [=](){initWS();registerCallback(*this);};

        registerCallback(lazyInit);
        rap=this;
    }

    void onCallback()
    {
        if (!m_initialized)
        {
        	std::cout << "callback calling Init" << m_i++ << std::endl;
            initWS();
        }
		uWSPlugin::singleLoopRun((us_loop_t *)uWS::Loop::get());
        registerCallback(*this);
	}

    void initWS()
    {
		/* Overly simple hello world app */
		m_loop = new uWS::App();
        m_loop->get("/favicon.ico", [&](auto *res, auto *req) {
            // send a TravelLogEvent to Pong
			

            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->end("Response is :favicon");
        });
        m_loop->get("/*", [&](auto *res, auto *req) {
            // send a TravelLogEvent to Pong
			

            res->writeHeader("Content-Type", "text/html; charset=utf-8");
            res->write("Response is :");
            res->onAborted([&](){std::cout<<"onAbort"<<std::endl;});
            m_pipe.push<RequestMsgEvent>(res, std::string(req->getUrl()), std::string(req->getMethod()), std::string(req->getQuery()));
		});
        uWS::TemplatedApp<false>::WebSocketBehavior wsBehaviour;
        wsBehaviour.compression = uWS::SHARED_COMPRESSOR;
        wsBehaviour.maxPayloadLength = 16 * 1024;
        wsBehaviour.idleTimeout = 10;
        wsBehaviour.open = [this](auto *ws, auto *req){
            std::cout << "WS: received open\n";
        };
        wsBehaviour.message = [this](auto *ws, std::string_view message, uWS::OpCode opCode)
        {
            std::cout << "WS: received data: " << message << "\n";
        };
        m_loop->ws<PerSocketData>("/*",std::move(wsBehaviour));
        m_loop->listen(3000, [](auto *token) {
			if (token) {
			std::cout << "Listening on port " << 3000 << std::endl;
			}
		});
        
        uWSPlugin::integrate((us_loop_t *)uWS::Loop::get());
        m_initialized=true;
    }

    // upon receiving a TravelLogEvent 
    void onEvent(const ResponseMsgEvent& e)
	{
        e.m_target->tryEnd(e.m_data);
	}

};


//---- MAIN --------------------------------------------------------------------

int main()
{
	std::cout << "tutorial #15 : integrating uWebSocket, non intrusive way" << std::endl;
    
    Engine::StartSequence	startSequence;
	
    // add BackEndActor, as service, on CPU core #2
	startSequence.addServiceActor<BackEndTag, BackEndActor>(2);
    // add RestActor on CPU core #1
	startSequence.addActor<RestActor>(1);

	Engine	engine(startSequence);
	
	sleep(200);
	
	return 0;
}
