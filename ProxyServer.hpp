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
	ProxyServer(const string & ip,const char * config_file, unsigned short port, size_t threadNum=8, size_t queSize=1024)
	: _acceptor(ip, port)
	, _loop(_acceptor,config_file, threadNum, queSize)
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
