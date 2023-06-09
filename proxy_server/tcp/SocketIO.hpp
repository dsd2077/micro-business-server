#ifndef __WD_SOCKETIO_H__
#define __WD_SOCKETIO_H__

#include <iostream>

using std::string;

namespace wd
{
    //封装读写操作
class SocketIO
{
public:
	SocketIO(int fd): _fd(fd) {}

	bool readn(string &buff);
	int writen(const char * buff, int len);
private:

private:
	int _fd;
};

}//end of namespace wd

#endif
