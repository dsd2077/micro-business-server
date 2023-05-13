#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <netdb.h>

class Client_Function {
public:
    Client_Function() {}

    virtual ~Client_Function() {}

public:
    virtual void ClientFunction(int nConnectedSocket) = 0;
};

class TCP_Client {
public:
    TCP_Client(Client_Function *pObserver, int nServerPort, const char *strServerIP) {
        m_pObserver = pObserver;

        m_nServerPort = nServerPort;

        int nlength = strlen(strServerIP);
        m_strServerIP = new char[nlength + 1];
        strcpy(m_strServerIP, strServerIP);
    }

    virtual ~TCP_Client() {
        delete[] m_strServerIP;
    }

public:
    int Run() {
        //创建套接字
        int nClientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == nClientSocket) {
            std::cout << "socket error" << std::endl;
            return -1;
        }

        // 设置服务器地址信息
        sockaddr_in ServerAddress;
        memset(&ServerAddress, 0, sizeof(sockaddr_in));     //TODO:为什么要memset?
        ServerAddress.sin_family = AF_INET;
//        ServerAddress.sin_addr.s_addr = inet_addr(m_strServerIP);   //域名地址
        ServerAddress.sin_port = htons(m_nServerPort);              //htons : host to network short将主机字节序转为网络字节序
        //设置ip地址
        if (::inet_pton(AF_INET, m_strServerIP, &ServerAddress.sin_addr) != 1) {
            std::cout << "inet_pton error" << std::endl;
            ::close(nClientSocket);
            return -1;
        }


        int ret = connect(nClientSocket, (sockaddr *) &ServerAddress, sizeof(ServerAddress));
        if (ret == -1) {
            perror("connect");
            ::close(nClientSocket);
            return -1;
        }

        std::cout << "连接成功" << std::endl;

        if (m_pObserver != NULL) {
            m_pObserver->ClientFunction(nClientSocket);
        }

        ::close(nClientSocket);

        return 0;
    }

private:
    int m_nServerPort;
    char *m_strServerIP;
    Client_Function *m_pObserver;
};

class My_Client_Function : public Client_Function {
public:
    My_Client_Function() {}

    virtual ~My_Client_Function() {}

private:
    virtual void ClientFunction(int nConnectedSocket) {
        while(1) {
            std::string command = "";
            getline(std::cin, command);
            command += "\n";

            send(nConnectedSocket, command.c_str(), command.length(), 0);

            char res[4096] = "";
            recv(nConnectedSocket, res, sizeof(res), 0);

            std::cout << res;
        }
    }
};

int gethostIP(const char * hostName) {
    struct hostent *he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(hostName)) == NULL) {
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for (int i = 0; addr_list[i] != NULL; i++) {
        std::cout << "IP Address " << i+1 << ": " << inet_ntoa(*addr_list[i]) << std::endl;
    }
    return 0;
}

int main()
{
    My_Client_Function client;
    TCP_Client tcpclient(&client, 10003, "8.130.85.224");
//    TCP_Client tcpclient(&client, 10003, "region-42.seetacloud.com");
    tcpclient.Run();
    return 0;
}

