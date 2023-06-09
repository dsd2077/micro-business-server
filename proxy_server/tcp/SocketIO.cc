#include "SocketIO.hpp"

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <iostream>
using std::cout;
using std::endl;
using std::string;
 
namespace wd
{

//循环读取,直到缓冲区为空
bool SocketIO::readn(string &buf)
{
	char temp[256];
	memset(temp, 0, sizeof(temp));
	while (true) {
		int n  = recv(_fd, temp, sizeof(temp)-1, 0);
		if (n == -1 and (errno == EAGAIN || errno == EWOULDBLOCK)) {
			break;
		}
		else if(n < 0) {
			perror("ERROR reading from socket");
			return false;
		}
		else if (n == 0) {
			//对端关闭
			return false;
		}
		buf += temp;
		memset(temp, 0, sizeof(temp));
	}
	return true;
	
}

int SocketIO::writen(const char * buff, int len)
{
	int left = len;//还剩下left个字节数没有发送到
	const char * p = buff;

	int ret = -1;
	while(left > 0) {
		/* ret = send(_fd, p, left, 0); */
		ret = write(_fd, p, left);
		if(ret == -1 && errno == EINTR)
			continue;
		else if(ret == -1) {
			perror("send");
			return len - left;
		} else {
			p += ret;
			left -= ret;
		}
	}
	return len - left;
}

}//end of namespace wd
