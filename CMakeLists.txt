cmake_minimum_required(VERSION 3.19.2)
project(tiny_business_server)

set(CMAKE_CXX_STANDARD 11)

add_executable(server1 server1.cpp Socket.cc Acceptor.cc InetAddress.cc)
add_executable(server2 server2.cpp Socket.cc Acceptor.cc InetAddress.cc)
add_executable(proxy_server TestTcpServer.cc TcpConnection.cc Acceptor.cc Socket.cc EventLoop.cc InetAddress.cc SocketIO.cc Threadpool.cc TaskQueue.cc MutexLock.cc Condition.cc Thread.cc)
target_link_libraries(proxy_server pthread)
add_executable(client client.cpp)
