#ifndef __WD_EVENTLOOP_H__
#define __WD_EVENTLOOP_H__

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "../threadpool/Threadpool.hpp"

namespace wd
{
class Acceptor;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class EventLoop
{
	using EventList = std::vector<struct epoll_event>;
	using TcpConnsMap = std::map<int, TcpConnectionPtr>;
public:
    //1 创建一个epoll句柄
    //2 将listenfd添加入监听事件
	EventLoop(Acceptor &, size_t threadNum = 8, size_t queSize=1024);
	~EventLoop();

    //调用waitEpollfd去开启epoll监听
	void loop();
	void unloop();		

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
	int setnonblock(int fd);

private:
	int         _efd;   //epoll 监听套接字
	Acceptor &  _acceptor;
	bool        _isLooping;
	EventList   _evtList;   //
	TcpConnsMap _conns;     //Tcp连接
    Threadpool _threadpool;
};

}//end of namespace wd

#endif
