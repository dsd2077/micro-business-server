#include "ProxyServer.hpp"
#include "./threadpool/Threadpool.hpp"
#include "config.h"

#include <iostream>
#include <sstream>

using std::cout;
using std::endl;
using namespace wd;

void test0() 
{
    Parser parser;
    parser.parse("../proxyserver.conf");
    ProxyServer server(parser);     
    server.start();
} 

int main(void)
{
    test0();
    return 0;
}
