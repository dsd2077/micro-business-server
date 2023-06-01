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

void TcpConnection::process()
{
	//读取消息
	_sockIO.readn(_read_buf, sizeof(_read_buf));
	cout << "read message : \n" << _read_buf << endl;
	//解析消息
	//通过http状态机

	//根据商品的类别信息转发消息

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
