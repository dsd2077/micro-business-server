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
	using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
	using TcpConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
public:
	TcpConnection(int fd);

	string receive();
	void send(const string & msg);
	bool isClosed() const;

	string toString() const;

	void setConnectionCallback(const TcpConnectionCallback & cb);
	void setMessageCallback(const TcpConnectionCallback & cb);
	void setCloseCallback(const TcpConnectionCallback & cb);

	void handleConnectionCallback();
	void handleMessageCallback();
	void handleCloseCallback();

private:
	InetAddress getLocalAddr();
	InetAddress getPeerAddr();

public:
    int total_money;
private:
	Socket _sock;       //对端的socket
	SocketIO _sockIO;   //封装读写操作的类
	InetAddress _localAddr;     //本端的地址
	InetAddress _peerAddr;      //对端的地址
	bool _isShutdownWrite;      //是否写关闭

	TcpConnectionCallback _onConnectionCb;
	TcpConnectionCallback _onMessageCb;
	TcpConnectionCallback _onCloseCb;

};



}//end of namespace wd

#endif
