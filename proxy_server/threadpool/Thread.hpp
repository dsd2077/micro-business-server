#ifndef __WD_THREAD_H__
#define __WD_THREAD_H__

#include <pthread.h>

#include <functional>
using std::function;

namespace wd
{
class Thread
{
public:
	using ThreadCallback = function<void()>;
	Thread(ThreadCallback cb)
	: _pthid(0)
	, _isRunning(false)
	, _cb(cb)  //注册回调函数
	{}

	~Thread();

	void start();
	void join();

private:
	//不希望该方法在类之外直接调用
	//同时要消除this指针的影响
	static void * threadFunc(void*);

private:
	pthread_t _pthid;
	bool _isRunning;
	ThreadCallback _cb;
};

}//end of nanmespace wd

#endif
