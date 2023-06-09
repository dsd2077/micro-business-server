#include "sql_connection_pool.h"

MYSQL *connection_pool::GetConnection()
{
    reserve.wait();
    auto conn = connList.back();
    connList.pop_back();
    return conn;
}

bool connection_pool::ReleaseConnection(MYSQL *conn)
{
    lock.lock();
    connList.push_back(conn);
    lock.unlock();
    reserve.post();
    return false;
}

connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
	return &connPool;
}

bool connection_pool::init(string url, string user, string password, string database, int port, int MaxConn)
{
    for (int i=0; i< MaxConn; i++) {
        MYSQL * conn = NULL;
        conn = mysql_init(conn);
        if (conn == NULL) {
            cout << "MySQL Error" << endl;
            return false;
        }
        conn = mysql_real_connect(conn, url.c_str(), user.c_str(), password.c_str(), database.c_str(), port, NULL, 0);
        if (conn == NULL) {
            cout << "MySQL Error" << endl;
            return false;
        }
        connList.push_back(conn);
        reserve.post();
    }

    return true;

}

connection_pool::~connection_pool()
{
    list<MYSQL *>::iterator it;
    for (it = connList.begin(); it != connList.end(); ++it) {
        MYSQL *con = *it;
        mysql_close(con);
    }
    connList.clear();
}

connectionRAII::connectionRAII(MYSQL **con, connection_pool *connPool)
{
    *con = connPool->GetConnection();
    conRAII = *con;
    poolRAII = connPool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}
