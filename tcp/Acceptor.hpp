#ifndef __WD_ACCEPTOR_H__
#define __WD_ACCEPTOR_H__

#include "Socket.hpp"
#include "InetAddress.hpp"


namespace wd
{
//TODO:这样的封装真的有意义吗？
//现在来看，确实没什么意义，从代码的复用性来讲
class Acceptor
{
public:
	Acceptor(const string & ip, unsigned short port);

	void ready();
	int accept();
	int fd() const {	return _listensock.fd();	}
private:
	void setReuseAddr();
	void setReusePort();
	void bind();
	void listen();
private:
	Socket _listensock;
	InetAddress _serverAddr;
};

}//end of namespace wd

#endif
