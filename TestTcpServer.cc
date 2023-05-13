#include "TcpServer.hpp"
#include "Threadpool.hpp"

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using namespace wd;

Threadpool * gThreadpool = nullptr;

void onConnection(const TcpConnectionPtr & conn) {
    cout << conn->toString() << " has connected!" << endl;
}

//process是子线程的处理函数
//对于我的业务来说，子线程需要处理客户端的业务请求，将业务转发给子服务器
class MyTask
{
public:
    MyTask(const string & msg,
           const TcpConnectionPtr & conn)
        : _msg(msg)
          , _conn(conn)
    {}

    //process方法的执行一定是在某一个计算线程中运行
    void process()
    {
        cout << "Task::process msg:" << _msg;
        //将_msg消息解析出来
        std::stringstream ss(_msg);
        string command = "";

        ss >> command;
        //购买商品
        if (command == "buy") {
            doBuyBussiness();
        }
        //显示有哪些商品
        else if(command == "show") {
            doShowBusiness();
        }
        //购买商品
        else if(command == "cash") {
            doCashBusiness();
        }

    }
    void doCashBusiness() {
        string s = "";
        s = std::to_string(_conn->total_money) + "\n";
        _conn->send(s);
        _conn->total_money = 0;
    }

    void doShowBusiness()
    {
        string s = "";
        for (auto &elem : gThreadpool->_belongs) {
            s += elem.first;
            s += "\n";
        }

        _conn->send(s);
    }
    void doBuyBussiness() {
        std::stringstream ss(_msg);
        //解析命令
        string command = "";
        ss >> command;
        string commodity = "";
        ss >> commodity;

        //查询该商品属于哪个业务服务器
        auto it = gThreadpool->_belongs.find(commodity);
        //商品不存在
        if (it == gThreadpool->_belongs.end()) {
            _conn->send("the goods does not exit");
            return;
        }

        std::cout << "commodity : " << commodity << std::endl;
        int fd = it->second;

        //该服务器已经挂掉，业务已经转移到备用服务器
        if (!gThreadpool->_isAlive[fd]) {
            changeToAnotherServer();
            return;
        }

        send(fd, _msg.c_str(), _msg.length(), 0);
        char buf[64] = {0};
        int ret = recv(fd, buf, sizeof(buf), 0);
        //该服务器已经挂掉
        if (ret == 0) {
            gThreadpool->_isAlive[fd] = false;
            changeToAnotherServer();
            return;
        }
        std::cout << "the price : " << buf;
        _conn->send(buf);
        _conn->total_money += atoi(buf);
    }

    void changeToAnotherServer() {
        send(gThreadpool->server_fd2, _msg.c_str(), _msg.length(), 0);
        char buf[64] = {0};
        int ret = recv(gThreadpool->server_fd2, buf, sizeof(buf), 0);

        _conn->send(buf);
        _conn->total_money += atoi(buf);
    }

private:
    string _msg;
    TcpConnectionPtr _conn;
};

//接收到客户端的消息后该怎么处理？
//1、decode————判断用户的目的：是买化妆品还是买买酒,例如 "buy mask" "buy wine"
//谁在调用onMessage()?————在EventLoop类中，当已经建立连接的描述符发来消息时，调用该函数进行处理
//我希望实现的框架：主线程只负责监听
void onMessage(const TcpConnectionPtr & conn) {
    string msg = conn->receive();
    MyTask task(msg, conn);
    gThreadpool->addTask(std::bind(&MyTask::process, task));
}

void onClose(const TcpConnectionPtr & conn){
    cout << conn->toString() << " has closed!" << endl;
}


void test0() 
{
    Threadpool threadpool(4, 10);
    threadpool.start();
    gThreadpool = &threadpool;
    /* Acceptor acceptor("192.168.30.128", 8888); */
    TcpServer server("", 10003);     //如果想要在公网上能够连接到该服务器，ip地址应该设置为公共IP地址
    server.setAllCallbacks(onConnection, onMessage, onClose);
    server.start();
} 

int main(void)
{
    test0();
    return 0;
}
