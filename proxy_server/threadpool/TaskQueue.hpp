#ifndef __WD_TASKQUEUE_H__
#define __WD_TASKQUEUE_H__

#include <queue>
#include <functional>
#include <iostream>
#include <mutex>
#include <condition_variable>

using std::queue;


namespace wd
{


template <typename ElemType>
class TaskQueue
{
public:
	TaskQueue(size_t sz);

	bool empty() const;
	bool full() const;
	void push(const ElemType & );
	ElemType pop();

	void wakeup();

private:
	size_t _queSize;
	queue<ElemType> _que;
	std::mutex _mutex;
	std::condition_variable _notFull;	
	std::condition_variable _notEmpty;
	bool _flag;		//_flag用来干嘛的？
};

template <typename ElemType>
TaskQueue<ElemType>::TaskQueue(size_t sz)
: _queSize(sz)
, _mutex()
, _flag(true)
{}

template <typename ElemType>
bool TaskQueue<ElemType>::empty() const
{
	return _que.size() == 0;
}

template <typename ElemType>
bool TaskQueue<ElemType>::full() const
{
	return _que.size() == _queSize;
}

//push函数运行在生产者线程
template <typename ElemType>
void TaskQueue<ElemType>::push(const ElemType & e)
{
	//RAII的技术解决死锁的问题
	std::unique_lock<std::mutex> guard(_mutex);
	while(full()) {			//为了防止出现虚假唤醒，必须使用while进行判断
		_notFull.wait(guard);
	}

	_que.push(e);
	//....return

	_notEmpty.notify_one();//通知消费者线程取数据
}

//pop函数运行在消费者线程
template <typename ElemType>
ElemType TaskQueue<ElemType>::pop()
{
	std::unique_lock<std::mutex> guard(_mutex);
	while(_flag && empty()) {
		_notEmpty.wait(guard);
	}

	//先从队列中获取数据,再pop
	if(_flag) {
		ElemType tmp = _que.front();
		_que.pop();
		//....return
		_notFull.notify_one();
		return tmp;
	} else {
		return 0;
	}
}

template <typename ElemType>
void TaskQueue<ElemType>::wakeup()
{
	_flag = false;
	_notEmpty.notify_all();
}
}//end of namespace wd

#endif
