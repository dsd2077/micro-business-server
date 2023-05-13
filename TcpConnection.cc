#include "TcpConnection.hpp"

#include <iostream>
#include <sstream>
using std::cout;
using std::endl;
using std::ostringstream;
 

namespace wd
{
TcpConnection::TcpConnection(int fd)
: _sock(fd)
, _sockIO(fd)
, _localAddr(getLocalAddr())
, _peerAddr(getPeerAddr())
, _isShutdownWrite(false)
{
}

string TcpConnection::receive()
{
	char buff[65535] = {0};
	_sockIO.readline(buff, sizeof(buff));
	return string(buff);
}

void TcpConnection::send(const string & msg)
{
	_sockIO.writen(msg.c_str(), msg.size());
}

string TcpConnection::toString() const
{
	ostringstream oss;
	oss << _localAddr.ip() << ":" << _localAddr.port() 
		<< " --> "
	    << _peerAddr.ip() << ":" <<  _peerAddr.port();
	return oss.str();
}

bool TcpConnection::isClosed() const
{
	int nret = -1;
	char buff[128] = {0};
	do {
		nret = ::recv(_sock.fd(), buff, sizeof(buff), MSG_PEEK);
	}while(nret == -1 && errno == EINTR);

	return nret == 0;
}

void TcpConnection::setConnectionCallback(const TcpConnectionCallback & cb)
{
	_onConnectionCb = std::move(cb);
}

void TcpConnection::setMessageCallback(const TcpConnectionCallback & cb)
{
	_onMessageCb = std::move(cb);
}

void TcpConnection::setCloseCallback(const TcpConnectionCallback & cb)
{
	_onCloseCb = std::move(cb);
}

void TcpConnection::handleConnectionCallback()
{
	if(_onConnectionCb) {
		_onConnectionCb(shared_from_this());
	}
}

void TcpConnection::handleMessageCallback()
{
	if(_onMessageCb) {
		_onMessageCb(shared_from_this());
	}
}

void TcpConnection::handleCloseCallback()
{
	if(_onCloseCb) {
		_onCloseCb(shared_from_this());
	}
}

InetAddress TcpConnection::getLocalAddr()
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	if(getsockname(_sock.fd(), (struct sockaddr*)&addr, &len) < 0) {
		perror("getsockname");
	}
	return InetAddress(addr);
}

InetAddress TcpConnection::getPeerAddr()
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	if(getpeername(_sock.fd(), (struct sockaddr*)&addr, &len) < 0) {
		perror("getsockname");
	}
	return InetAddress(addr);
}


}//end of namespace wd
