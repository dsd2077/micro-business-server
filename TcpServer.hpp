#ifndef __WD_TCPSERVER_H__
#define __WD_TCPSERVER_H__

#include "./tcp/Acceptor.hpp"
#include "./event_loop/EventLoop.hpp"
#include "./tcp/TcpConnection.hpp"

namespace wd
{

class TcpServer
{
public:
	TcpServer(const string & ip, unsigned short port, size_t threadNum=8, size_t queSize=1024)
	: _acceptor(ip, port)
	, _loop(_acceptor, threadNum, queSize)
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
