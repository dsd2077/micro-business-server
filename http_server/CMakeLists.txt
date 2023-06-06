cmake_minimum_required(VERSION 3.19.2)
project(httpserver)

set(CMAKE_CXX_STANDARD 11)

add_executable(httpserver_test HttpServer_test.cc)
target_link_libraries(httpserver_test muduo_http muduo_net muduo_base mysqlclient jsoncpp)