#include "Thread.hpp"
#include <stdio.h>

namespace wd
{

Thread::~Thread()
{
	if(_isRunning) {
		pthread_detach(_pthid);
	}
}

//start函数中是有this指针
void Thread::start()
{
	if(pthread_create(&_pthid, nullptr, threadFunc, this)) {
		perror("pthread_create");
		return;
	}

	_isRunning = true;
}

//该函数是静态的，没有this指针;
//所以只能通过传递参数来达到目的
void * Thread::threadFunc(void * arg)
{
	//子线程入口函数要执行的任务就是run所代表的
	Thread * pthread = static_cast<Thread*>(arg);
	if(pthread) {
		pthread->_cb(); //调用回调函数
	}

	return nullptr;
}

void Thread::join()
{
	if(_isRunning) {
		pthread_join(_pthid,nullptr);
		_isRunning = false;
	}
}


}// end of namesapce wd
