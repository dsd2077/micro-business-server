#include "Acceptor.hpp"

#include <unistd.h>

namespace wd
{

Acceptor::Acceptor(unsigned short port)
: _listensock()
, _serverAddr("0.0.0.0", port)
{}

void Acceptor::ready()
{
	//TODO:没有设置边缘触发也没有设置非阻塞
	setReuseAddr();
	setReusePort();
	bind();
	listen();
}

int Acceptor::accept()
{
	int peerfd = ::accept(fd(), nullptr, nullptr);
	if(peerfd < 0) {
		perror("accept");
	}
	return peerfd;
}

void Acceptor::setReuseAddr()
{
	int on = 1;
	if(setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		perror("setsockopt");
	}
}

void Acceptor::setReusePort()
{
	int on = 1;
	if(setsockopt(fd(), SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
		perror("setsockopt");
	}
}

void Acceptor::bind()
{
	if(::bind(fd(), 
		(struct sockaddr*)_serverAddr.getInetAddressPtr(), 
		sizeof(InetAddress)) < 0) {
		perror("bind");
		//文件描述符是比较稀缺的，所以不用的时候要回收
	}
}
	
void Acceptor::listen()
{
	if(::listen(fd(), 128) < 0) {
		perror("listen");
	}
}

}//end of namespace wd
