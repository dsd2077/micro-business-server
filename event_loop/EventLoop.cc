#include "EventLoop.hpp"
#include "../tcp/TcpConnection.hpp"
#include "../tcp/Acceptor.hpp"

#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace wd
{

EventLoop::EventLoop(Acceptor & acceptor, size_t threadNum, size_t queSize)
: _efd(createEpollfd())
, _acceptor(acceptor)
, _isLooping(false)
, _evtList(1024)
, _threadpool(threadNum, queSize)
{
	addEpollReadFd(_acceptor.fd());
}
	
EventLoop::~EventLoop()
{
	if(_efd) {
		close(_efd);
	}
}


void EventLoop::loop()
{
	_threadpool.start();
	_isLooping = true;
	while(_isLooping) {
		waitEpollfd();
	}
}

void EventLoop::unloop()
{
	_isLooping = false;
}

void EventLoop::waitEpollfd()
{
	int nready = 0;
	do {
		nready = ::epoll_wait(_efd, &*_evtList.begin(), _evtList.size(), 5000);
	}while(nready == -1 && errno == EINTR);

	if(nready == -1) {
		perror("epoll_wait");
		return;
	} else if(nready == 0) {
//		printf(">> epoll_wait timeout!\n");
        return;
	} else {
		//nready > 0
		if(nready == _evtList.size()) {
			_evtList.resize(2 * nready);
		}
		for(int idx = 0; idx < nready; ++idx) {
			int fd = _evtList[idx].data.fd;
			if(fd == _acceptor.fd() &&
                    (_evtList[idx].events == EPOLLIN)) {
				handleNewConnection();
			} else {
                if(_evtList[idx].events == EPOLLIN) {
					handleMessage(fd);
				}
			}
			//TODO:为什么这里没有监听写事件？——因为采用的是Reactor模式，读写都在子线程中进行，所以在子线程就进行了send
		}
	}
}

//新连接到达，如何将连接交给子线程处理？
void EventLoop::handleNewConnection() {
	int peerfd = _acceptor.accept();

	addEpollReadFd(peerfd);
    //下面应该交给子线程进行处理
	TcpConnectionPtr conn(new TcpConnection(peerfd));
	_conns.insert(std::make_pair(peerfd, conn));	
	//TODO:将下列语句改为日志记录
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	if(getpeername(peerfd, (struct sockaddr*)&addr, &len) < 0) {
		perror("getsockname");
	}
	std::cout << "new connection come in , ip : " << string(inet_ntoa(addr.sin_addr)) << std::endl;
}

void EventLoop::handleMessage(int fd)
{
	std::cout << "数据可读 fd : "  << fd << std::endl;
	auto iter = _conns.find(fd);
	_threadpool.addRequest(iter->second);
}


int EventLoop::createEpollfd()
{
	int fd = epoll_create1(0);
	if(fd < 0) {
		perror("epoll_create1");
	}
	return fd;
}


void EventLoop::addEpollReadFd(int fd)
{
	//没有设置边沿触发和非阻塞
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = fd;
	int ret = ::epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev);
	if(ret < 0) {
		perror("epoll_ctl");
	}
	setnonblock(fd);
}

void EventLoop::delEpollReadFd(int fd)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	int ret = ::epoll_ctl(_efd, EPOLL_CTL_DEL, fd, &ev);
	if(ret < 0) {
		perror("epoll_ctl");
	}
}

int EventLoop::setnonblock(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
}//end of namespace wd
