#include "TaskQueue.hpp"

namespace wd
{

TaskQueue::TaskQueue(size_t sz)
: _queSize(sz)
, _que()
, _mutex()
, _notFull(_mutex)
, _notEmpty(_mutex)
, _flag(true)
{}

bool TaskQueue::empty() const
{
	return _que.size() == 0;
}

bool TaskQueue::full() const
{
	return _que.size() == _queSize;
}

//push函数运行在生产者线程
void TaskQueue::push(const ElemType & e)
{
	//RAII的技术解决死锁的问题
	MutexLockGuard autolock(_mutex);
	while(full()) {			//为了防止出现虚假唤醒，必须使用while进行判断
		_notFull.wait();
	}

	_que.push(e);
	//....return

	_notEmpty.notify();//通知消费者线程取数据
}

//pop函数运行在消费者线程
ElemType TaskQueue::pop()
{
	MutexLockGuard autolock(_mutex);
	while(_flag && empty()) {
		_notEmpty.wait();
	}

	//先从队列中获取数据,再pop
	if(_flag) {
		ElemType tmp = _que.front();
		_que.pop();
		//....return
		_notFull.notify();
		return tmp;
	} else {
		return nullptr;
	}
}

void TaskQueue::wakeup()
{
	_flag = false;
	_notEmpty.notifyAll();
}



}//end of namespace wd
