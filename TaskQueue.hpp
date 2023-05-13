#ifndef __WD_TASKQUEUE_H__
#define __WD_TASKQUEUE_H__

#include "MutexLock.hpp"
#include "Condition.hpp"
#include "Task.hpp"

#include <queue>
#include <functional>
using std::queue;


namespace wd
{

using ElemType  = std::function<void()>; 

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
	MutexLock _mutex;
	Condition _notFull;
	Condition _notEmpty;
	bool _flag;
};

}//end of namespace wd

#endif
