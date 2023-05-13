#include "EventLoop.hpp"
#include "TcpConnection.hpp"
#include "Acceptor.hpp"

#include <unistd.h>
#include <iostream>

namespace wd
{

EventLoop::EventLoop(Acceptor & acceptor)
: _efd(createEpollfd())
, _acceptor(acceptor)
, _isLooping(false)
, _evtList(1024)
, _threadpool(10, 10)

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
	_isLooping = true;
	while(_isLooping) {
		waitEpollfd();
	}
}

void EventLoop::unloop()
{
	_isLooping = false;
}

void EventLoop::setConnectionCallback(TcpConnectionCallback && cb)
{
	_onConnectionCb = std::move(cb);
}

void EventLoop::setMessageCallback(TcpConnectionCallback && cb)
{
	_onMessageCb = std::move(cb);
}

void EventLoop::setCloseCallback(TcpConnectionCallback && cb)
{
	_onCloseCb = std::move(cb);
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
		}
	}
}

//新连接到达，如何将连接交给子线程处理？
void EventLoop::handleNewConnection() {
	int peerfd = _acceptor.accept();

	addEpollReadFd(peerfd);
    //下面应该交给子线程进行处理
	TcpConnectionPtr conn(new TcpConnection(peerfd));

	conn->setConnectionCallback(_onConnectionCb);
	conn->setMessageCallback(_onMessageCb);
	conn->setCloseCallback(_onCloseCb);

	_conns.insert(std::make_pair(peerfd, conn));
	//触发新连接到来时的事件处理器
	conn->handleConnectionCallback();
}

void EventLoop::handleMessage(int fd)
{

    //1. 通过fd找到TcpConnection对象
	auto iter = _conns.find(fd);
	if(iter != _conns.end()) {

        //2. 判断该连接是否断开
		bool isColsed = iter->second->isClosed();
		//2.1 如果连接断开，执行连接断开时的事件处理器
		if(isColsed) {
			iter->second->handleCloseCallback();
			delEpollReadFd(fd);
			_conns.erase(iter);
		} else {

            //2.2 如果连接没有断开，执行消息到达时的事件处理器
			iter->second->handleMessageCallback();
		}
	}
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
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;
	int ret = ::epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev);
	if(ret < 0) {
		perror("epoll_ctl");
	}
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
}//end of namespace wd
