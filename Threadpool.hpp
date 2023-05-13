#ifndef __WD_THREADPOOL_H__
#define __WD_THREADPOOL_H__

#include "TaskQueue.hpp"
#include "Thread.hpp"
#include "Task.hpp"

#include <vector>
#include <memory>
#include <map>

using std::vector;
using std::unique_ptr;


namespace wd
{

class Threadpool
{
public:
	Threadpool(size_t threadNum, size_t queSize);
	~Threadpool();
	void start();
	void stop();
	void addTask(Task &&);
private:
	Task getTask();
	void threadFunc();
    int connectToServer(const char *ip, const int port);
    void loadGoodsFromServer(int server_fd);
public:
    int server_fd1;     //服务器1的套接字
    int server_fd2;     //服务器2的套接字
    std::map<int, bool> _isAlive;

    std::map<std::string,int> _belongs;     //商品属于那一台服务器

private:
	size_t _threadNum;
	size_t _queSize;

	vector<unique_ptr<Thread>> _threads;
	TaskQueue _taskque;
	bool _isExit;

};

}//end of namespaced wd


#endif



