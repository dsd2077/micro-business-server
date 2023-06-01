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
		unique_ptr<Thread> up(new Thread(std::bind(&Threadpool::threadFunc, this)));        
		_threads.push_back(std::move(up));
	}
	for(auto & pthread : _threads) {
		pthread->start();
	}
    //加载每台服务器所销售的商品
}

void Threadpool::addRequest(TcpConnPtr conn)
{
	_taskque.push(conn);
}

TcpConnPtr Threadpool::getRequest()
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
	std::cout << "线程启动 ： " << pthread_self() << std::endl;
	while(!_isExit) {
		TcpConnPtr conn = getRequest();
		if(conn) {
			conn->process();// 执行任务的地方
		}
	}
}


}//end of namespace wd
