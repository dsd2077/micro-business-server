#include "ProxyServer.hpp"
#include "./threadpool/Threadpool.hpp"

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using namespace wd;

void test0() 
{
    //TODO:这里不再设置ip和端口号，通过配置文件进行设置
    ProxyServer server("", "../proxyserver.conf", 10001);     
    server.start();
} 

int main(void)
{
    test0();
    return 0;
}
