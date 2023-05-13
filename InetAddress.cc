#include "InetAddress.hpp"


#include <strings.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>



#include <iostream>
using std::cout;
using std::endl;
 
namespace wd
{

InetAddress::InetAddress(const string & ip, unsigned short port)
{
	::bzero(&_addr, sizeof(struct sockaddr_in));
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	_addr.sin_addr.s_addr = ip=="" ? INADDR_ANY : ::inet_addr(ip.c_str());
}

InetAddress::InetAddress(const struct sockaddr_in & addr)
: _addr(addr)
{}

string InetAddress::ip() const
{
	return string(inet_ntoa(_addr.sin_addr));
}
	
unsigned short InetAddress::port() const
{
	return ntohs(_addr.sin_port);
}

const struct sockaddr_in * InetAddress::getInetAddressPtr() const
{
	return &_addr;
}

}// end of namespace wd
