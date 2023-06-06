#include "TcpConnection.hpp"

#include <string.h>
#include <iostream>
#include <sstream>
#include <sstream>
using std::cout;
using std::endl;
using std::ostringstream;
 

namespace wd
{
TcpConnection::TcpConnection(int fd, CONNECTION_TYPE type, 
							map<string, vector<int>> &forwardingTable, TcpConnsMap &serverConns,
							map<string, int> &clientTable, TcpConnsMap &clientConns)
: _sock(fd)
, _sockIO(fd)
, _localAddr(getLocalAddr())
, _peerAddr(getPeerAddr())
, _isShutdownWrite(false)
, _type(type)
, _forwardingTable(forwardingTable)
, _serverConns(serverConns)
, _clientTable(clientTable)
, _clientConns(clientConns)
{
}

void TcpConnection::send(const string & msg)
{
	_sockIO.writen(msg.c_str(), msg.size());
}

string TcpConnection::toString() const
{
	ostringstream oss;
	oss << _localAddr.ip() << ":" << _localAddr.port() 
		<< " --> "
	    << _peerAddr.ip() << ":" <<  _peerAddr.port();
	return oss.str();
}

void TcpConnection::process()
{
	if (_type == SERVER) {
		do_server2client();
	}
	else if(_type == CLIENT) {
		do_client2server();
	}
	//根据商品的类别信息转发消息
	//解析出商品category
	//现在在子线程中，
	//方案一：获取到业务服务器的链接，在这里发送消息
	//方案二：通知将业务服务器的fd设置为EPOLLOUT，不在这里发送消息（读写分离）
	//方案三：从配置中控制转发
}

void TcpConnection::do_server2client()
{
	if (!_sockIO.readn(_read_buf)) {
		//TODO:读取出错，对端断开连接
	}
	//解析http请求，从中取出Client字段
	int beg_pos = _read_buf.find("Client: ");
	if (beg_pos == string::npos) return;
	beg_pos += 8;
	int end_pos = _read_buf.find_first_of('\n', beg_pos + 1);
	if (end_pos == string::npos) return;
	string client = _read_buf.substr(beg_pos, end_pos-beg_pos-1);

	if (_clientTable.find(client) == _clientTable.end()) {
		return;
	}
	int fd = _clientTable[client];
	_clientConns[fd]->send(_read_buf);

	_read_buf = "";
}

void TcpConnection::do_client2server()
{
	if (!_sockIO.readn(_read_buf)) {
		//对端已经关闭，关闭链接
		cout << "对端断开链接" << endl;
		_clientConns.erase(_sock.fd());
		_clientTable.erase(_peerAddr.ip_port());
		return;
	}
	//TDOO:解析消息——不需要完整的解析，只需要解析出请求行就可以了
	// 在Header中添加Client信息，即原ip和端口信息。业务服务器在响应请求时不修改Client信息，
	std::istringstream iss(_read_buf);
	string line;
	std::getline(iss, line);		//getline会从/n截断，所以会留下/r

	if (parse_request_line(line) == BAD_REQUEST) {
		//TODO:错误处理
		return;
	} 
	//在消息中插入一个head字段 Client: ip:port
	string new_message = _read_buf;
	int header_pos = new_message.find_first_of("\n");
	new_message.insert(header_pos+1, "Client: " + _peerAddr.ip_port() + "\r\n");
	if (_forwardingTable.find(_business_code) == _forwardingTable.end() || _forwardingTable[_business_code].size() == 0) {
		//TODO:错误处理
		return;
	}
	int fd = _forwardingTable[_business_code].front();
	_serverConns[fd]->send(new_message);

	_read_buf = "";
}

HTTP_CODE TcpConnection::parse_request_line(string line)
{
	line = line.substr(0, line.size()-1);
	int blank_pos1 = line.find_first_of(" ");
	if (blank_pos1 == string::npos) {
		return BAD_REQUEST;
	}
	string method = line.substr(0, blank_pos1);
	if (method != "GET" && method != "POST") {
		return BAD_REQUEST;
	}
	int blank_pos2 = line.rfind(" ");
	if (blank_pos2 == blank_pos1) {
		return BAD_REQUEST;
	}
	string path = line.substr(blank_pos1+1, blank_pos2-blank_pos1-1);
	//这里要将业务代码截出来，如果不存在该业务就将其设置BAD_REQUEST
	_business_code = path.substr(0, path.find_first_of('/', 1)+1);
	if (_forwardingTable.find(_business_code) == _forwardingTable.end()) {
		return BAD_REQUEST;
	}
	
	string protocol = line.substr(blank_pos2 + 1);
	if (protocol != "HTTP/1.1" && protocol != "HTTP/1.0") {
		return BAD_REQUEST;
	}
    return GET_REQUEST;
}

bool TcpConnection::isClosed() const
{
	int nret = -1;
	char buff[128] = {0};
	do {
		nret = ::recv(_sock.fd(), buff, sizeof(buff), MSG_PEEK);
	}while(nret == -1 && errno == EINTR);

	return nret == 0;
}

InetAddress TcpConnection::getLocalAddr()
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	if(getsockname(_sock.fd(), (struct sockaddr*)&addr, &len) < 0) {
		perror("getsockname");
	}
	return InetAddress(addr);
}

InetAddress TcpConnection::getPeerAddr()
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(struct sockaddr_in);
	if(getpeername(_sock.fd(), (struct sockaddr*)&addr, &len) < 0) {
		perror("getsockname");
	}
	return InetAddress(addr);
}


}//end of namespace wd
