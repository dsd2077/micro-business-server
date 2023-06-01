#include "SocketIO.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


#include <iostream>
using std::cout;
using std::endl;
 
namespace wd
{

//循环读取,直到缓冲区为空
int SocketIO::readn(char * buff, int len)
{
	int left = len; // 还剩下left个字节数没有获取到
	char *p = buff;

	int ret = -1;
	while (left > 0)
	{
		/* ret = recv(_fd, p, left, 0); */
		ret = read(_fd, p, left);
		if (ret == -1) 
		{
			if (errno == EINTR) continue;		// The call was interrupted by a signal before any data was read
			else if (errno == EAGAIN || errno == EWOULDBLOCK) break;
		}
		else if (ret == 0) break;						//(zero indicates end of file)对端已关闭连接
		else
		{
			p += ret;
			left -= ret;
		}
	}
	return len - left;
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

//这里一定会读取maxlen个字节吗？
int SocketIO::readline(char * buff, int maxlen)
{
	int left = maxlen - 1;//留一个字节的空间存放'\0'
	char * p = buff;
	int ret = -1;
	int total = 0;
	while(left > 0) {
		//MSG_PEEK预取内核接收缓冲区中的数据，而不移走数据
		ret = recv(_fd, p, left, MSG_PEEK);
		if(ret == -1 && errno == EINTR)
			continue;
		else if(ret == -1) {
			perror("recv");
		} else if(ret == 0) {
			break;
		} else {
			//ret > 0
			for(int idx = 0; idx < ret; ++idx) {
				//2. 找到了 \n
				if(p[idx] == '\n') {
					int sz = idx + 1;
					readn(p, sz);
					p += sz;
					*p = '\0';
					return total + sz; 
				}
			}
			//1. 没有发现 \n ,继续往后找
			readn(p, ret);
			left -= ret;
			total += ret;
			p += ret;
		}//end else
	}//end while
	//3. 找了maxlen - 1 个字节的长度，都没有发现 \n
	*p = '\0';
	return total - left;
}


}//end of namespace wd
