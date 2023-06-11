#ifndef __WD_TCPCONNECTION_H__
#define __WD_TCPCONNECTION_H__

#include "Noncopyable.hpp"
#include "Socket.hpp"
#include "SocketIO.hpp"
#include "InetAddress.hpp"

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <jsoncpp/json/json.h>

using std::string;
using std::map;
using std::vector;


namespace wd
{
enum HTTP_CODE {
	BAD_REQUEST,
	GET_REQUEST,
};

enum HttpStatusCode
{
    Unknown,
    Ok = 200,
    MovedPermanently = 301,
    BadRequest = 400,
    NotFound = 404,
};

enum CONNECTION_TYPE {
	CLIENT,
	SERVER,
};

class TcpConnection 
: Noncopyable
, public std::enable_shared_from_this<TcpConnection>
{
	using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
	using TcpConnsMap = map<int, TcpConnectionPtr>;

public:
	TcpConnection(int fd, CONNECTION_TYPE type,
				map<string,vector<int>> &forwardingTable, TcpConnsMap &serverConns,
				map<string, int> &clientTable, TcpConnsMap &clientConns);

	string receive();
	void send(const string & msg);
	bool isClosed() const;
	string toString() const;
	void process();		
	void do_server2client();
	void do_client2server();
	HTTP_CODE parse_request_line(string line);


private:
	InetAddress getLocalAddr();
	InetAddress getPeerAddr();
    bool find_forward_path();
    void error_handle(HttpStatusCode code, const string message, Json::Value data);

private:
	Socket _sock;       		//对端的socket
	SocketIO _sockIO;   		//封装读写操作的类
	InetAddress _localAddr;     //本端的地址
	InetAddress _peerAddr;      //对端的地址
	bool _isShutdownWrite;      //是否写关闭
	string _read_buf;			//消息缓冲区
	CONNECTION_TYPE _type;		//连接类型,client or server
	map<string, vector<int>> &_forwardingTable;			//client2server转发表 <业务编号，[服务器fd]>
	map<string, int> &_clientTable;						//server2client转发表 <ip:port, fd>
	TcpConnsMap &_serverConns;	    //业务服务器Tcp连接
	TcpConnsMap &_clientConns;		//客户端Tcp连接

    //http protocol related variable
    string _path;               //http path
	string _business_code;			//业务编号
};
}//end of namespace wd

#endif
