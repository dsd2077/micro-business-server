#ifndef __WD_TCPSERVER_H__
#define __WD_TCPSERVER_H__

#include "./tcp/Acceptor.hpp"
#include "./event_loop/EventLoop.hpp"
#include "./tcp/TcpConnection.hpp"

namespace wd
{

class ProxyServer
{
public:
	ProxyServer(Parser &config)
	: _acceptor(config.server.listen)
	, _loop(_acceptor,config)
	{}

	void start()
	{
		_acceptor.ready();
		_loop.loop();
	}

	void stop()
	{
		_loop.unloop();
	}

private:

private:
	Acceptor _acceptor;
	EventLoop _loop;    //EventLoop
};

}//end of namespace wd

#endif
