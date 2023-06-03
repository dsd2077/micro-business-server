#ifndef __WD_EVENTLOOP_H__
#define __WD_EVENTLOOP_H__

#include <sys/epoll.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "../threadpool/Threadpool.hpp"
#include "../config.h"

namespace wd
{
class Acceptor;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class EventLoop
{
	using EventList = std::vector<struct epoll_event>;		//这两个类型定义为什么放在类之内，而TcpConnectionPtr放在类之外
	using TcpConnsMap = std::map<int, TcpConnectionPtr>;
public:
    //1 创建一个epoll句柄
    //2 将listenfd添加入监听事件
	EventLoop(Acceptor &,const char * config_file, size_t threadNum = 8, size_t queSize=1024);
	~EventLoop();

    //调用waitEpollfd去开启epoll监听
	void loop();
	void unloop();		

private:
	void waitEpollfd();
	void handleNewConnection();
	void handleMessage(int);
	bool connectToBusinessServer();
	int connectToOneBusinessServer(const char * ip, int port);

	int createEpollfd();
	void addEpollReadFd(int fd);
	void delEpollReadFd(int fd);
	int setnonblock(int fd);

private:
	int         _efd;   //epoll 监听套接字
	Acceptor &  _acceptor;
	bool        _isLooping;
	EventList   _evtList;   //
	TcpConnsMap _clientConns;     //客户Tcp连接
	TcpConnsMap _serverConns;	  //业务服务器Tcp连接
    Threadpool _threadpool;
	Parser _config;
	std::map<std::string, int> _clientTable;						//server2client转发表<ip, fd>
	std::map<std::string, std::vector<int>> _forwardingTable;		//client2server转发表<业务编号，[服务器fd]>
};

}//end of namespace wd

#endif
