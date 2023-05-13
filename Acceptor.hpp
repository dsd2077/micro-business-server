#ifndef __WD_ACCEPTOR_H__
#define __WD_ACCEPTOR_H__

#include "Socket.hpp"
#include "InetAddress.hpp"


namespace wd
{

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
