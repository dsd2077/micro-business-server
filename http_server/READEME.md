### 安装依赖：

~~~shell
#cmake
sudo apt-get install cmake

#muduo网络库,要像博客中那样将头文件和静态库放在系统路径下
这个有一点麻烦，参考：https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980

# jsoncpp
sudo apt-get install libjsoncpp-dev
~~~



### 配置

在server1.cc和server2.cc文件中修改数据库配置：

const char *username = "root";
const char *password = "123456";
const char *database = "database_name";



### 编译

~~~shell
mkdir build
cmake ..
~~~

### 个性化运行

~~~shell
./server1 [-p port] [-s sql_num] [-t thread_num] 
~~~

- p，自定义端口号
  - 默认10001
- -s，数据库连接数量
  - 默认为8
- -t，线程数量
  - 默认为8

### 测试示例命令

./server1 -p 10003 -s 8 -t 2

./server2 -p 10002 -s 8 -t 2