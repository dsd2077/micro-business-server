cmake_minimum_required(VERSION 3.19.2)
project(httpserver)

set(CMAKE_CXX_STANDARD 11)

add_executable(server1 server1.cc HttpServer.cc HttpResponse.cc HttpContext.cc sql_connection_pool.cpp config.cpp)
add_executable(server2 server2.cc HttpServer.cc HttpResponse.cc HttpContext.cc sql_connection_pool.cpp config.cpp)
target_link_libraries(server1 muduo_net muduo_base mysqlclient jsoncpp pthread)
target_link_libraries(server2 muduo_net muduo_base mysqlclient jsoncpp pthread)