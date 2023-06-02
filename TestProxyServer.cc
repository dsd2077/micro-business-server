#include "ProxyServer.hpp"
#include "./threadpool/Threadpool.hpp"

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using namespace wd;

void test0() 
{
    ProxyServer server("", "./proxyserver.conf", 10001);     //如果想要在公网上能够连接到该服务器，ip地址应该设置为公共IP地址
    server.start();
} 

int main(void)
{
    test0();
    return 0;
}
