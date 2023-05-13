#include "Threadpool.hpp"

#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <sstream>

namespace wd
{

Threadpool::Threadpool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)
, _queSize(queSize)
, _taskque(_queSize)
, _isExit(false)
{
	_threads.reserve(_threadNum);
}

Threadpool::~Threadpool()
{
	if(!_isExit) {
		stop();
	}
}

void Threadpool::start()
{
	for(size_t idx = 0; idx != _threadNum; ++idx) {
		unique_ptr<Thread> up(new Thread(
			std::bind(&Threadpool::threadFunc, this)));
		_threads.push_back(std::move(up));
	}
	for(auto & pthread : _threads) {
		pthread->start();
	}
    server_fd1 = connectToServer("127.0.0.1", 10000);
    server_fd2 = connectToServer("127.0.0.1", 10001);
    //加载每台服务器所销售的商品
    loadGoodsFromServer(server_fd1);
    loadGoodsFromServer(server_fd2);
    _isAlive[server_fd1] = true;
    _isAlive[server_fd2] = true;
}
void Threadpool::loadGoodsFromServer(int server_fd) {
    std::string s = "getGoodsInfo\n";
    int ret = send(server_fd, s.c_str(), s.length(), 0);
    if(ret == -1) {
        perror("send");
        exit(-1);
    }

    char buf[4096];
    ret = recv(server_fd, buf, sizeof(buf), 0);
    if(ret == -1) {
        perror("recv");
        exit(-1);
    }

    std::stringstream ss(buf);
    std::string goods;
    while(ss >> goods) {
        _belongs[goods] = server_fd;
    }
}

int Threadpool::connectToServer(const char *ip, const int port) {
    int nClientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == nClientSocket) {
        std::cout << "socket error" << std::endl;
        return -1;
    }

    sockaddr_in ServerAddress;
    memset(&ServerAddress, 0, sizeof(sockaddr_in));
    ServerAddress.sin_family = AF_INET;
    if (::inet_pton(AF_INET, ip, &ServerAddress.sin_addr) != 1) {
        std::cout << "inet_pton error" << std::endl;
        ::close(nClientSocket);
        return -1;
    }

    ServerAddress.sin_port = htons(port);

    if (::connect(nClientSocket, (sockaddr *) &ServerAddress, sizeof(ServerAddress)) == -1) {
        std::cout << "connect error" << std::endl;
        ::close(nClientSocket);
        return -1;
    }
    return nClientSocket;
}


void Threadpool::addTask(Task && task)
{
	if(task) {
		_taskque.push(std::move(task));
	}
}

Task Threadpool::getTask()
{
	return _taskque.pop();
}


//运行在主线程
void Threadpool::stop()
{
	//在退出线程池之前，要确保任务都已经完成了
	while(!_taskque.empty()) {
		sleep(1);
	}

	_isExit = true;
	_taskque.wakeup();
	for(auto & pthread : _threads) {
		pthread->join();
	}
}

//每一个工作线程需要做的事情
void Threadpool::threadFunc()
{
	while(!_isExit) {
		Task taskcb = getTask();
		if(taskcb) {
			taskcb();// 执行任务的地方
		}
	}
}


}//end of namespace wd
