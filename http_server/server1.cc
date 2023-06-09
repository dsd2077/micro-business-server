#include "./HttpRequest.h"
#include "./HttpResponse.h"
#include "./HttpServer.h"
#include "./sql_connection_pool.h"
#include "muduo/base/Logging.h"
#include "muduo/net/EventLoop.h"
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>

using namespace muduo;
using namespace muduo::net;
using std::cout;
using std::endl;
using std::lock_guard;
using std::mutex;

bool benchmark = false;

const char *username = "dsd";
const char *password = "520134";
const char *database = "commodity";

connection_pool * sql_conn_pool;        //数据库连接池
mutex conn_mutex;                       //用于操作数据库时加锁

class Task
{
public:
    void onRequest(const HttpRequest &req, HttpResponse *resp)
    {
        std::cout << "Headers " << req.methodString() << " " << req.path() << " " << req.getVersion() << std::endl;
        const std::map<string, string> &headers = req.headers();
        for (const auto &header : headers) {
            std::cout << header.first << ": " << header.second << std::endl;
        }
        if (req.path() == "/10001/buy/") {
            do_buy(req, resp);
        } else if (req.path() == "/10001/show/") {
            do_show(req, resp);
        } 
    }   

    void do_buy(const HttpRequest &req, HttpResponse *resp)
    {
        // 查询数据库，商品存在,返回成功
        string id = req.getQuery("commodity_id");
        if (id == "")
            return;
        string num = req.getQuery("num");
        if (num == "")
            return;
        MYSQL * conn;
        connectionRAII(&conn, sql_conn_pool);

        lock_guard<mutex> guard(conn_mutex);
        string query = "SELECT * FROM cosmetics WHERE id=" + id;
        if (mysql_query(conn, query.c_str())) {         //我们的问题
            cout << "查询数据库失败: " << mysql_error(conn) << endl;
            error_handle(resp, HttpResponse::kUnknown, "网络出错", req.getHeader("Client"));
            return;
        }
        MYSQL_RES *result = mysql_store_result(conn); // 数据库出错,我们的问题
        if (!result) {
            cout << "获取查询结果失败: " << mysql_error(conn) << endl;
            error_handle(resp, HttpResponse::kUnknown, "网络出错", req.getHeader("Client"));
            return;
        }
        MYSQL_ROW row = mysql_fetch_row(result);
        if (row == NULL) {                              //有可能是客户的问题,有可能是链接失效了，有可能是商品下架了
            cout << "该商品不存在" << endl;
            error_handle(resp, HttpResponse::k400BadRequest, "未找到商品", req.getHeader("Client"));
            return;
        }
        int stock = atoi(row[2]); // 假设第三列是库存量
        if (stoi(num) > stock) {
            cout << "商品数量不足" << endl;
            error_handle(resp, HttpResponse::k400BadRequest, "商品数量不足", req.getHeader("Client"));
            return;
        }
        query = "UPDATE cosmetics SET num=num-" + num + " WHERE id=" + id;
        if (mysql_query(conn, query.c_str())) // mysql服务器的问题
        {
            cout << "更新数据库失败: " << mysql_error(conn) << endl;
            error_handle(resp, HttpResponse::kUnknown, "网络出错", req.getHeader("Client"));
            return;
        }

        Json::Value item;
        item["price"] = stoi(num) * atof(row[3]);
        sucess_handle(resp, HttpResponse::k200Ok, "OK", item, req.getHeader("Client"));

        mysql_free_result(result);
    }

    void sucess_handle(HttpResponse *resp, HttpResponse::HttpStatusCode code, const string message, Json::Value item, const string client)
    {
        resp->setStatusCode(code);
        resp->setStatusMessage(message);
        resp->setContentType("application/json");
        resp->addHeader("Client", client);

        Json::Value data;
        data["code"] = HttpResponse::k200Ok;
        data["message"] = message;
        data["data"] = item;

        Json::StreamWriterBuilder writerBuilder;
        std::string outputStr = Json::writeString(writerBuilder, data);  // 将 Value 对象转换为字符串
        resp->setBody(outputStr);
    }

    void error_handle(HttpResponse *resp, HttpResponse::HttpStatusCode code, const string message, const string client) 
    {
        resp->setStatusCode(code);
        resp->setStatusMessage(message);
        resp->addHeader("Client", client);
        resp->setContentType("text/plain;charset=UTF-8");
        Json::Value data;
        data["code"] = code;
        data["message"] = message;
        Json::FastWriter writer;
        string json_str = writer.write(data); // 将 JSON 对象转换为 JSON 字符串
        resp->setBody(json_str); 
    }

    void do_show(const HttpRequest &req, HttpResponse *resp)
    {
        // 查询数据库
        const char *sql_query = "select * from cosmetics";
        MYSQL * conn;
        connectionRAII(&conn, sql_conn_pool);
        if (mysql_query(conn, sql_query)) {
            cout << "mysql_query error" << endl;
            error_handle(resp, HttpResponse::kUnknown, "网络出错", req.getHeader("Client"));
            return;
        }

        MYSQL_RES *result = mysql_store_result(conn);
        if (!result) {
            cout << "获取查询结果失败: " << mysql_error(conn) << endl;
            error_handle(resp, HttpResponse::kUnknown, "网络出错", req.getHeader("Client"));
            return;
        }

        MYSQL_FIELD *fields = mysql_fetch_fields(result);
        // Create JSON object and array
        Json::Value items(Json::arrayValue);
        while (MYSQL_ROW row = mysql_fetch_row(result)) {
            Json::Value item;
            item["id"] = row[0];
            item["name"] = row[1];
            item["num"] = row[2];
            item["price"] = row[3];
            item["des"] = row[4];
            items.append(item);
        }

        mysql_free_result(result);
        Json::FastWriter writer;
        sucess_handle(resp, HttpResponse::k200Ok, "OK", items, req.getHeader("Client"));
    }
};



int main(int argc, char *argv[])
{
    int numThreads = 0;
    if (argc > 1) {
        benchmark = true;
        Logger::setLogLevel(Logger::WARN);
        numThreads = atoi(argv[1]);
    }
    EventLoop loop;
    HttpServer server(&loop, InetAddress(10003), "dummy");
    Task t;
    server.setHttpCallback(std::bind(&Task::onRequest, t, _1, _2));

    server.setThreadNum(numThreads);

    sql_conn_pool = connection_pool::GetInstance();
    sql_conn_pool->init("localhost", username, password, database, 3306, 8);

    server.start();
    loop.loop();
}
