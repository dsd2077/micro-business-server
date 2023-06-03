#include "tcp/Acceptor.hpp"

#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <signal.h>
#include <functional>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>

using namespace wd;
using std::string;
using std::cout;
using std::endl;

//商品类
struct Goods{
    Goods(const std::string n, int p, int number)
    :name(n), price(p), num(number){}

    void display() {
        std::cout << name << "   " << price << "   " << num << std::endl;
    }
    std::string name;
    int price;
    int num;
};

class Server_Function
{
public:
    Server_Function(){}

    virtual ~Server_Function(){}

public:
    virtual void ServerFunction(int nConnectedSocket, int nListenSocket) = 0;

    int serializing(const std::string &file_path){
        size_t len = sizeof(*this);
        std::ofstream o(file_path);

        o.write((char *)this, len);
        o.close();
        return 0;
    }

    std::vector<Goods> _items;
};

class TCP_Server {
public:
    using EventList = std::vector<struct epoll_event>;

    TCP_Server(Server_Function *pObserver, int nServerPort, const char *strBoundIP = NULL,int nLengthOfQueueOfListen = 100)
               : m_pObserver(pObserver)
               ,_acceptor(strBoundIP, nServerPort)
               ,_efd(createEpollfd())
               ,_evtList(1024)
               {}

    virtual ~TCP_Server() {
        if (m_pObserver) {
            delete m_pObserver;
        }
        if (_efd) {
            close(_efd);
        }
        if (_another_server) {
            delete _another_server;
        }
    }

public:
    int Run() {
        _acceptor.ready();
        addEpollReadFd(_acceptor.fd());
        waitEpollfd();

        return 0;
    }

private:
    void waitEpollfd()
    {
        while(1) {
            int nready = 0;
            do {
                nready = ::epoll_wait(_efd, &*_evtList.begin(), _evtList.size(), 5000);
            } while (nready == -1 && errno == EINTR);

            if (nready == -1) {
                perror("epoll_wait");
                return;
            } else if (nready == 0) {
                //超时
                continue;
            } else {
                //nready > 0
                if (nready == _evtList.size()) {
                    _evtList.resize(2 * nready);
                }

                for (int idx = 0; idx < nready; ++idx) {
                    int fd = _evtList[idx].data.fd;
                    if (fd == _acceptor.fd() &&_evtList[idx].events == EPOLLIN) {
                        int peer_fd = _acceptor.accept();
                        addEpollReadFd(peer_fd);
                    }
                    //代理服务器发来信息
                    else {
                        m_pObserver->ServerFunction(fd, _efd);
                    }
                }
            }
        }
    }

    void displayGoods() {
        std::cout << "name" << "   " << "price" << "   " << "num" << std::endl;
        for(auto &g : m_pObserver->_items) {
            g.display();
        }
    }

    void transferGoods(const std::vector<Goods> & temp) {
        for(auto &g : temp) {
            m_pObserver->_items.push_back(g);
        }
    }

    int createEpollfd()
    {
        int fd = epoll_create1(0);
        if(fd < 0) {
            perror("epoll_create1");
        }
        return fd;
    }

    int setnonblocking(int fd) {
        int old_option = fcntl(fd, F_GETFL);
		int new_option = old_option | O_NONBLOCK;
		fcntl(fd, F_SETFL, new_option);
		return old_option;
    }

    void addEpollReadFd(int fd)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        int ret = ::epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev);
        if(ret < 0) {
            perror("epoll_ctl");
        }
        setnonblocking(fd);
    }

    void delEpollReadFd(int fd)
    {
        struct epoll_event ev;
        ev.data.fd = fd;
        int ret = ::epoll_ctl(_efd, EPOLL_CTL_DEL, fd, &ev);
        if(ret < 0) {
            perror("epoll_ctl");
        }
    }

private:
    Acceptor _acceptor;
    Server_Function *m_pObserver;
    int _efd;
    EventList   _evtList;
    Server_Function * _another_server = nullptr;
};

class My_Server_Function : public Server_Function {
public:
    My_Server_Function() {}

    virtual ~My_Server_Function() {}

private:
    virtual void ServerFunction(int nConnectedSocket, int epollSocket) {
        char buf[512];
        int ret = recv(nConnectedSocket, buf, sizeof(buf), 0);
        if (ret == -1) {
            perror("recv");
            return;
        }
        if (ret == 0) {
            std::cout << "peer closed" << std::endl;
            ret = close(nConnectedSocket);
            ret = epoll_ctl(epollSocket, EPOLL_CTL_DEL, nConnectedSocket, NULL);
            return;
        }
        cout << "recv message : " << buf << endl;
        std::stringstream ss(buf);
        std::string command;

        // ss >> command;
        // std::cout << "command : " << command << std::endl;
        // string reply;
        // if (command == "getGoodsInfo") {
        //     reply = getGoodsInfo();
        // } else if (command == "buy") {
        //     string goods;
        //     ss >> goods;
        //     std::cout << "goods : " << goods << std::endl;

        //     //显然对于商品来说，将Goods存在vector里面并不是一个很好的选择，最好是存在map里面，不过要改成map的话太麻烦了，懒得改了
        //     for (auto &g: _items) {
        //         if (g.name == goods && g.num > 0) {
        //             --g.num;
        //             reply = std::to_string(g.price) + "\n";
        //             break;
        //         }
        //     }
        //     if (reply == "") {
        //         reply = "no such goods or the goods was saled out\n";
        //     }
        // }
        // send(nConnectedSocket, reply.c_str(), reply.length(), 0);
    }
    std::string getGoodsInfo() {
        string s;
        for (auto &g: _items) {
            s += g.name;
            s += " ";
        }
        return s;
    }
};

int main()
{
    My_Server_Function myserver;
    TCP_Server tcpserver(&myserver, 10003, "127.0.0.1");

    tcpserver.Run();
    return 0;
}

