#ifndef __WD_EVENTLOOP_H__
#define __WD_EVENTLOOP_H__

#include <sys/epoll.h>

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include "Threadpool.hpp"
namespace wd
{

class Acceptor;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TcpConnectionCallback = std::function<void(const TcpConnectionPtr&)>;

class EventLoop
{
	using EventList = std::vector<struct epoll_event>;
	using TcpConnsMap = std::map<int, TcpConnectionPtr>;
public:
    //1 创建一个epoll句柄
    //2 将listenfd添加入监听事件
	EventLoop(Acceptor &);
	~EventLoop();

    //调用waitEpollfd去开启epoll监听
	void loop();
	void unloop();

	void setConnectionCallback(TcpConnectionCallback && cb);
	void setMessageCallback(TcpConnectionCallback && cb);
	void setCloseCallback(TcpConnectionCallback && cb);
private:

    //调用epoll_wait开启监听
    //等待epoll返回
    //1 listenfd可读
    //2 客户端可读
	void waitEpollfd();
	void handleNewConnection();
	void handleMessage(int);

	int createEpollfd();
	void addEpollReadFd(int fd);
	void delEpollReadFd(int fd);


private:

	int         _efd;   //epoll 监听套接字
	Acceptor &  _acceptor;
	bool        _isLooping;
	EventList   _evtList;   //
	TcpConnsMap _conns;     //Tcp连接

	TcpConnectionCallback _onConnectionCb;
	TcpConnectionCallback _onMessageCb;
	TcpConnectionCallback _onCloseCb;

    Threadpool _threadpool;
	
};

}//end of namespace wd

#endif
