#include "TcpConnection.hpp"

#include <string.h>
#include <iostream>
#include <sstream>
#include <sstream>
#include <algorithm>
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
}

void TcpConnection::do_server2client()
{
	if (!_sockIO.readn(_read_buf)) {
		cout << "服务器断开链接" << endl;
		_serverConns.erase(_sock.fd());
		for (auto &pair : _forwardingTable) {
			auto pos = std::find(pair.second.begin(), pair.second.end(), _sock.fd());
			if (pos != pair.second.end()) {
				pair.second.erase(pos);
			}
		}
	}
    cout << "message : " << endl << _read_buf << endl;
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
	// 在Header中添加Client信息，即原ip和端口信息。业务服务器在响应请求时不修改Client信息，
	std::istringstream iss(_read_buf);
	string line;
	std::getline(iss, line);		//getline会从/n截断，所以会留下/r

	if (parse_request_line(line) == BAD_REQUEST) {
        Json::Value data;
        data["code"] = 404;
        data["message"] = "404 Not Found";
        error_handle(HttpStatusCode::NotFound, "404 Not Found", data);
		return;
	} 

	//在消息中插入一个head字段 Client: ip:port
	string new_message = _read_buf;
	int header_pos = new_message.find_first_of("\n");
	new_message.insert(header_pos+1, "Client: " + _peerAddr.ip_port() + "\r\n");
	if (_forwardingTable[_business_code].size() == 0) {
        Json::Value data;
        data["code"] = 404;
        data["message"] = "404 Not Found";
        error_handle(HttpStatusCode::NotFound, "404 Not Found", data);
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
	_path = line.substr(blank_pos1+1, blank_pos2-blank_pos1-1);
    if (!find_forward_path()) {
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

bool TcpConnection::find_forward_path()
{
    int pos = _path.size()-1;
    while (pos > 0) {
        if (_forwardingTable.find(_path.substr(0, pos+1)) != _forwardingTable.end()) {
            _business_code = _path.substr(0, pos+1);
            return true;
        }
        if (_path[pos] == '/') {
            pos -= 1;
        } else {
            pos = _path.find_last_of('/', pos);
        }
    }

    return false;
}

void TcpConnection::error_handle(HttpStatusCode code, const string message, Json::Value data)
{
    string buffer;
    Json::StreamWriterBuilder writerBuilder;
    std::string body = Json::writeString(writerBuilder, data);  // 将 Value 对象转换为字符串
    char buf[32];
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", code);
    buffer += buf;
    buffer += message;
    buffer += "\r\n";

    //添加头部信息
    buffer += "Connection: close\r\n";
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body.size());
    buffer += buf;
    buffer += "Content-Type: application/json\r\n";

    buffer += "\r\n";
    buffer += body;
    _sockIO.writen(buffer.c_str(), buffer.size());
}

}//end of namespace wd
