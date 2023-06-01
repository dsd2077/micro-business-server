#ifndef __WD_SOCKETIO_H__
#define __WD_SOCKETIO_H__


namespace wd
{
    //封装读写操作
class SocketIO
{
public:
	SocketIO(int fd): _fd(fd) {}

	int readn(char * buff, int len);
	int readline(char * buff, int maxlen);
	int writen(const char * buff, int len);
private:

private:
	int _fd;
};

}//end of namespace wd

#endif
