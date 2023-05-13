#include "Acceptor.hpp"

#include <netinet/in.h>
#include <string.h>
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

std::function<void(int)> sighandler;
void callSighandler(int sig) {
    sighandler(sig);
}

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

    TCP_Server(Server_Function *pObserver, int nServerPort, const char *strBoundIP = NULL,
               int nLengthOfQueueOfListen = 100)
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
        if (_server_fd) {
            close(_server_fd);
        }
    }

public:
    int Run() {
        sighandler = std::bind(&TCP_Server::signalDeal,  this, std::placeholders::_1);
        signal(SIGINT,callSighandler);   //注册SIGINT对应的处理函数
        _acceptor.ready();
        addEpollReadFd(_acceptor.fd());
        connectToAnotherBusinessServer("127.0.0.1", 10000);     //TODO:从配置文件读取另一个服务器的地址

        //添加出售商品，以后可以考虑从数据库或者文件中读取
        m_pObserver->_items.push_back(Goods("wine", 200, 50));
        m_pObserver->_items.push_back({"red wine", 2000, 5});
        m_pObserver->_items.push_back({"vodka", 500, 50});
        m_pObserver->_items.push_back({"maotai", 1500, 50});

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
                    if (fd == _acceptor.fd() &&
                        (_evtList[idx].events == EPOLLIN)) {
                        int peer_fd = _acceptor.accept();
                        addEpollReadFd(peer_fd);
                    }
                        //主服务器发来消息，表示主服务器需要备份
                    else if (fd == _server_fd) {
                        if (_evtList[idx].events == EPOLLIN) {
                            //对方已经挂了，必须把连接断开，否则会一直触发epoll_wait
                            close(_server_fd);
                            std::vector<Goods> temp;
                            temp = deserializing("server.obj");  //反序列化
                            std::cout << "deserializing success" << std::endl;

                            //将商品倒过来
                            transferGoods(temp);
                        }
                    }
                        //代理服务器发来信息
                    else {
                        m_pObserver->ServerFunction(fd, _acceptor.fd());
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

    void signalDeal(int sig)
    {
        if(sig == SIGINT)    //对应ctrl+c
        {
            serializing("server.obj");
            //通知另一台服务器进行反序列化
            send(_server_fd, "1", 1, 0);
            exit(-1);
        }
    }

    int serializing(const std::string &file_path){
        m_pObserver->serializing(file_path);
        return 0;
    }

    static std::vector<Goods> deserializing(const std::string &file_path){
        std::ifstream is(file_path);
        //如何在不知道文件长度的情况下读取整个文件
        //1 用get/getline循环读取，直到文件末尾
        //2 先偏移到文件末尾，获取文件的长度，然后用read读取
        //3 使用输出运算符
        std::vector<Goods> temp;
        std::string line;
        while(getline(is, line)) {
            std::stringstream ss(line);
            std::string name;
            ss >> name;
            int num;
            ss >> num;
            int price;
            ss >> price;
            temp.push_back({name, price, num});
        }

        is.close();
        return temp;
    }

    int createEpollfd()
    {
        int fd = epoll_create1(0);
        if(fd < 0) {
            perror("epoll_create1");
        }
        return fd;
    }

    int connectToAnotherBusinessServer(const char *ip, int port) {
        _server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == _server_fd) {
            std::cout << "socket error" << std::endl;
            return -1;
        }

        sockaddr_in ServerAddress;
        memset(&ServerAddress, 0, sizeof(sockaddr_in));
        ServerAddress.sin_family = AF_INET;
        if (::inet_pton(AF_INET, ip, &ServerAddress.sin_addr) != 1) {
            std::cout << "inet_pton error" << std::endl;
            ::close(_server_fd);
            return -1;
        }

        ServerAddress.sin_port = htons(port);

        if (::connect(_server_fd, (sockaddr *) &ServerAddress, sizeof(ServerAddress)) == -1) {
            std::cout << "connect error" << std::endl;
            ::close(_server_fd);
            return -1;
        }
        addEpollReadFd(_server_fd);
        return _server_fd;
    }
    void addEpollReadFd(int fd)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        int ret = ::epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &ev);
        if(ret < 0) {
            perror("epoll_ctl");
        }
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
    int _server_fd;  //另一台服务器的socket


};

class My_Server_Function : public Server_Function {
public:
    My_Server_Function() {}

    virtual ~My_Server_Function() {}

private:
    virtual void ServerFunction(int nConnectedSocket, int nListenSocket) {
        char buf[512];
        int ret = recv(nConnectedSocket, buf, sizeof(buf), 0);
        if (ret == -1) {
            perror("recv");
            return;
        }
        if (ret == 0) {
            std::cout << "peer closed" << std::endl;
            return;
        }
        std::stringstream ss(buf);
        std::string command;

        ss >> command;
        std::cout << "command : " << command << std::endl;
        string reply;
        if (command == "getGoodsInfo") {
            reply = getGoodsInfo();
        } else if (command == "buy") {
            string goods;
            ss >> goods;
            std::cout << "goods : " << goods << std::endl;

            //显然对于商品来说，将Goods存在vector里面并不是一个很好的选择，最好是存在map里面，不过要改成map的话太麻烦了，懒得改了
            for (auto &g: _items) {
                if (g.name == goods && g.num > 0) {
                    --g.num;
                    reply = std::to_string(g.price) + "\n";
                    break;
                }
            }
            if (reply == "") {
                reply = "no such goods or the goods was saled out\n";
            }
        }
        send(nConnectedSocket, reply.c_str(), reply.length(), 0);
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
    TCP_Server tcpserver(&myserver, 10001, "127.0.0.1");

    tcpserver.Run();
    return 0;
}

