#ifndef __WD_SOCKET_H__
#define __WD_SOCKET_H__

#include "Noncopyable.hpp"


namespace wd
{

class Socket
: Noncopyable
{
public:
	Socket();
	Socket(int fd);
	~Socket();

	int fd() const {	return _fd;	}	

	void shutdownWrite();

private:
	int _fd;
};

}//end of namespace wd

#endif
