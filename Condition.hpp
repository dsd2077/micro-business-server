#ifndef _WD_CONDITION_H__
#define _WD_CONDITION_H__

#include "Noncopyable.hpp"

#include <pthread.h>

namespace wd
{

class MutexLock;//类的前向声明(不完全类型)

class Condition : Noncopyable
{
public:
	Condition(MutexLock & mutex);
	~Condition();

	void wait();
	void notify();
	void notifyAll();

private:
	MutexLock & _mutex;//指针大小
	pthread_cond_t _cond;
};

}//end of namespace wd

#endif
