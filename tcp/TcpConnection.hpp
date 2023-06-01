#ifndef __WD_TCPCONNECTION_H__
#define __WD_TCPCONNECTION_H__

#include "Noncopyable.hpp"
#include "Socket.hpp"
#include "SocketIO.hpp"
#include "InetAddress.hpp"

#include <string>
#include <memory>
#include <functional>

using std::string;

namespace wd
{

class TcpConnection 
: Noncopyable
, public std::enable_shared_from_this<TcpConnection>
{
	static const int READ_BUFFER_SIZE = 2048;	

public:
	TcpConnection(int fd);

	string receive();
	void send(const string & msg);
	bool isClosed() const;
	string toString() const;
	void process();		//

private:
	InetAddress getLocalAddr();
	InetAddress getPeerAddr();

private:
	Socket _sock;       //对端的socket
	SocketIO _sockIO;   //封装读写操作的类
	InetAddress _localAddr;     //本端的地址
	InetAddress _peerAddr;      //对端的地址
	bool _isShutdownWrite;      //是否写关闭
	char _read_buf[READ_BUFFER_SIZE];
};
}//end of namespace wd

#endif
