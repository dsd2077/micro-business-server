#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <mutex>
#include "sem.h"

using namespace std;

class connection_pool
{
public:
	MYSQL *GetConnection();				 //获取数据库连接
	bool ReleaseConnection(MYSQL *conn); //释放连接

	//单例模式
	static connection_pool *GetInstance();

	bool init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn); 

private:
	connection_pool(){};
	~connection_pool();

	mutex lock;
	list<MYSQL *> connList; //连接池
    sem reserve;
};

class connectionRAII{
public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif
