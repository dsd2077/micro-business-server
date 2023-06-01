#ifndef __WD_THREADPOOL_H__
#define __WD_THREADPOOL_H__

#include "TaskQueue.hpp"
#include "Thread.hpp"
#include "../tcp/TcpConnection.hpp"

#include <vector>
#include <memory>
#include <map>

using std::vector;
using std::unique_ptr;


namespace wd
{
using TcpConnPtr = std::shared_ptr<TcpConnection>;
class Threadpool
{
public:
	Threadpool(size_t threadNum, size_t queSize);
	~Threadpool();
	void start();
	void stop();
	void addRequest(TcpConnPtr conn);
	TcpConnPtr getRequest();

private:
	void threadFunc();

private:
	size_t _threadNum;
	size_t _queSize;

	vector<unique_ptr<Thread>> _threads;
	TaskQueue<TcpConnPtr> _taskque;
	bool _isExit;
};

}//end of namespaced wd


#endif



