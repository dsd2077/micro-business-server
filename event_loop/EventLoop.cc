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
#include <string.h>

namespace wd
{
	EventLoop::EventLoop(Acceptor &acceptor, const char *config_file, size_t threadNum, size_t queSize)
		: _efd(createEpollfd()), _acceptor(acceptor), _isLooping(false), _evtList(1024), _threadpool(threadNum, queSize)
	{
		addEpollReadFd(_acceptor.fd());
		_config.parse(config_file);
	}

	EventLoop::~EventLoop()
	{
		if (_efd)
		{
			close(_efd);
		}
	}

	void EventLoop::loop()
	{
		// 开启线程池
		connectToBusinessServer();
		_threadpool.start();
		_isLooping = true;
		while (_isLooping)
		{
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
		do
		{
			nready = ::epoll_wait(_efd, &*_evtList.begin(), _evtList.size(), 5000);
		} while (nready == -1 && errno == EINTR);

		if (nready == -1)
		{
			perror("epoll_wait");
			return;
		}
		else if (nready == 0)
		{
			//		printf(">> epoll_wait timeout!\n");
			return;
		}
		else
		{
			// nready > 0
			if (nready == _evtList.size())
			{
				_evtList.resize(2 * nready);
			}
			for (int idx = 0; idx < nready; ++idx)
			{
				int fd = _evtList[idx].data.fd;
				if (fd == _acceptor.fd() &&
					(_evtList[idx].events == EPOLLIN))
				{
					handleNewConnection();
				}
				else
				{
					if (_evtList[idx].events == EPOLLIN)
					{
						handleMessage(fd);
					}
				}
				// TODO:为什么这里没有监听写事件？——因为采用的是Reactor模式，读写都在子线程中进行，所以在子线程就进行了send
			}
		}
	}

	// 新连接到达，如何将连接交给子线程处理？
	void EventLoop::handleNewConnection()
	{
		int peerfd = _acceptor.accept();

		addEpollReadFd(peerfd);
		// 下面应该交给子线程进行处理
		TcpConnectionPtr conn(new TcpConnection(peerfd, CLIENT));
		_clientConns.insert(std::make_pair(peerfd, conn));
		// TODO:将下列语句改为日志记录
		struct sockaddr_in addr;
		socklen_t len = sizeof(struct sockaddr_in);
		if (getpeername(peerfd, (struct sockaddr *)&addr, &len) < 0)
		{
			perror("getsockname");
		}
		std::cout << "new connection come in , ip : " << string(inet_ntoa(addr.sin_addr)) << std::endl;
	}

	void EventLoop::handleMessage(int fd)
	{
		auto iter = _clientConns.find(fd);
		_threadpool.addRequest(iter->second);
	}

	bool EventLoop::connectToBusinessServer()
	{
		for (auto server : _config.servers)
		{
			for (const auto &loc : server.locations)
			{
				std::string location = loc.first;
				_forwardingTable[location] = vector<int>();
				std::string proxy_name = loc.second.proxy_pass.substr(7);		//截掉前面的http://
				cout << "连接业务服务器" << proxy_name << endl;
				for (const auto &upstream : _config.upstreams[proxy_name].servers)
				{
					int fd = connectToOneBusinessServer(upstream.first.c_str(), stoi(upstream.second));
					_forwardingTable[location].push_back(fd);
					addEpollReadFd(fd);
					TcpConnectionPtr conn(new TcpConnection(fd, SERVER));
					_serverConns.insert({fd, conn});
					cout << "连接服务器 ip: " << upstream.first.c_str() << " port : "<< stoi(upstream.second) << " fd : " << fd << std::endl;
				}
				cout << endl;
			}
		}
		return true;
	}

	int EventLoop::connectToOneBusinessServer(const char *ip, int port)
	{

		int nClientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (-1 == nClientSocket)
		{
			std::cout << "socket error" << std::endl;
			return -1;
		}

		sockaddr_in ServerAddress;
		memset(&ServerAddress, 0, sizeof(sockaddr_in));
		ServerAddress.sin_family = AF_INET;
		if (::inet_pton(AF_INET, ip, &ServerAddress.sin_addr) != 1)
		{
			std::cout << "inet_pton error" << std::endl;
			::close(nClientSocket);
			return -1;
		}

		ServerAddress.sin_port = htons(port);

		if (::connect(nClientSocket, (sockaddr *)&ServerAddress, sizeof(ServerAddress)) == -1)
		{
			std::cout << "connect error" << std::endl;
			::close(nClientSocket);
			return -1;
		}
		return nClientSocket;
	}

	int EventLoop::createEpollfd()
	{
		int fd = epoll_create1(0);
		if (fd < 0)
		{
			perror("epoll_create1");
		}
		return fd;
	}

	void EventLoop::addEpollReadFd(int fd)
	{
		struct epoll_event ev;
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = fd;
		int ret = ::epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev);
		if (ret < 0)
		{
			perror("epoll_ctl");
		}
		setnonblock(fd);
	}

	void EventLoop::delEpollReadFd(int fd)
	{
		struct epoll_event ev;
		ev.data.fd = fd;
		int ret = ::epoll_ctl(_efd, EPOLL_CTL_DEL, fd, &ev);
		if (ret < 0)
		{
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
} // end of namespace wd
