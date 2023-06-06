#include "muduo/net/http/HttpServer.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/EventLoop.h"
#include "muduo/base/Logging.h"
#include <mysql/mysql.h>
#include <jsoncpp/json/json.h>

#include <iostream>
#include <map>

using namespace muduo;
using namespace muduo::net;
using std::cout;
using std::endl;


bool benchmark = false;   

void onRequest(const HttpRequest& req, HttpResponse* resp)
{
  std::cout << "Headers " << req.methodString() << " " << req.path()  << " " << req.getVersion() << std::endl;
  if (!benchmark)
  {
    const std::map<string, string>& headers = req.headers();
    for (const auto& header : headers)
    {
      std::cout << header.first << ": " << header.second << std::endl;
    }
  }

  if (req.path() == "/")
  {
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/html");
    resp->addHeader("Server", "Muduo");
    string now = Timestamp::now().toFormattedString();
    resp->setBody("<html><head><title>This is title</title></head>"
        "<body><h1>Hello</h1>Now is " + now +
        "</body></html>");
  }
  else if (req.path() == "/favicon.ico")
  {
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("image/png");
    // resp->setBody(string(favicon, sizeof favicon));
  }
  else if (req.path() == "/hello")
  {
    resp->setStatusCode(HttpResponse::k200Ok);
    resp->setStatusMessage("OK");
    resp->setContentType("text/plain");
    resp->addHeader("Server", "Muduo");
    resp->setBody("hello, world!\n");
  }
  else
  {
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
  }
}

const char *username = "dsd";
const char *password = "520134";
const char *database = "commodity";

MYSQL *conn = NULL;
bool init_mysql() {
  conn = mysql_init(conn);
  if (conn == NULL) {
    cout << "MySQL Error" << endl;
    exit(1);
  }
  conn = mysql_real_connect(conn, "localhost", username, password, database, 3306, NULL, 0);
  if (conn == NULL) {
    cout << "MySQL Error" << endl;
  }
}


class Task {
public:
  void onRequest(const HttpRequest &req, HttpResponse *resp)
  {
    std::cout << "Headers " << req.methodString() << " " << req.path() << " " << req.getVersion() << std::endl;
    const std::map<string, string> &headers = req.headers();
    for (const auto &header : headers)
    {
      std::cout << header.first << ": " << header.second << std::endl;
    }
    if (req.path() == "/10001/buy/") {
      do_buy(req, resp);
    }
    else if (req.path() == "/10001/show/") {
      do_show(req, resp);
    }
  }

  void do_buy(const HttpRequest &req, HttpResponse *resp) {
      //查询数据库，商品存在,返回成功


  }

  void do_show(const HttpRequest &req, HttpResponse *resp) {
      //查询数据库
      const char *sql_query = "select * from cosmetics";
      if (mysql_query(conn, sql_query)) {
        cout << "mysql_query error"  << endl;
      }

      MYSQL_RES *result = mysql_store_result(conn);

      int num_fields = mysql_num_fields(result);

      MYSQL_FIELD *fields = mysql_fetch_fields(result);
      // Create JSON object and array
      Json::Value data;
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
      data["items"] = items;

      Json::StreamWriterBuilder builder;
      builder.settings_["indentation"] = "";
      std::string json_str = Json::writeString(builder, data);
      cout << json_str << endl;
      
      resp->setStatusCode(HttpResponse::k200Ok);
      resp->setStatusMessage("OK");
      resp->setContentType("application/json");
      resp->addHeader("Server", "Muduo");
      resp->addHeader("Client", req.getHeader("Client"));
      //将Client字段添加进来
      resp->setBody(json_str);
  }
};

int main(int argc, char* argv[])
{
  int numThreads = 0;
  ///TODO:为什么这么写？
  if (argc > 1)
  {
    benchmark = true;
    Logger::setLogLevel(Logger::WARN);
    numThreads = atoi(argv[1]);
  }
  EventLoop loop;
  HttpServer server(&loop, InetAddress(10003), "dummy");
  Task t = Task();
  server.setHttpCallback(std::bind(&Task::onRequest, t, _1, _2));
  server.setThreadNum(numThreads);
  server.start();
  init_mysql();
  loop.loop();
}
