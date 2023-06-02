#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <deque>
#include <memory>

using namespace std;

enum CONFIG_CODE {
    BEGAIN,
    SERVER,
    UPSTREAM,
    LOCATION,
};
enum LINE_STATUS {
    LINE_OK,
    LINE_BAD,
};

struct Location {
    std::string id;
    std::string proxy_pass;
};

struct Server {
    std::string listen;
    std::string server_name;
    std::map<std::string, Location> locations;
};

struct Upstream {
    std::string name;
    std::vector<pair<string, string>> servers;      //<ip, port>
};

class Parser {
private:
    string curLocation;
    string curUpstream;

    CONFIG_CODE check_state = BEGAIN;
    LINE_STATUS line_state = LINE_OK;

    bool server_left_bracket = false;
    bool location_left_bracket = false;
    bool upstream_left_bracket = false;


public:
    std::vector<Server> servers;  //可能有多个server
    std::map<std::string, Upstream> upstreams;  //可能有多个代理服务器

    void parse(const std::string &filename) {

        std::ifstream file(filename);
        std::string line;
        std::string word;
        while (std::getline(file, line) && line_state == LINE_OK) {
            line = trim(line);
            if (line.empty() || line.front() == '#') {
                continue;
            }
            //找到第一个单词
            switch (check_state)
            {
            case BEGAIN:
            {
                istringstream iss(line);
                iss >> word;
                if (word == "server")
                {
                    check_state = SERVER;
                    servers.emplace_back(Server());
                    parse_bracket(iss, server_left_bracket);
                    break;
                }
                else if (word == "upstream") {
                    check_state = UPSTREAM;
                    Upstream stream = Upstream();
                    iss >> stream.name;
                    curUpstream = stream.name;
                    upstreams.insert({curUpstream, stream});
                    parse_bracket(iss, upstream_left_bracket);
                    break;
                }
                else {
                    throw std::runtime_error("expected server or upstream block");
                    return;
                }
            }
            case SERVER:
            {
                line_state = parse_server(line);
                break;
            }
            case LOCATION:
            {
                line_state = parse_location(line);
                break;
            }
            case UPSTREAM:
            {
                line_state = parse_upstream(line);
                break;
            }
            }
        }
        if (line_state != LINE_OK || check_state != BEGAIN) {
            check_state = BEGAIN;
            throw std::runtime_error("Unexpected end of configuration file");
        }

    }

private:
    LINE_STATUS parse_bracket(istringstream &iss, bool &bracket)
    {
        string word;
        if (iss >> word)
        {
            if (word == "{")
            {
                bracket = true;
            }
            else
            {
                return LINE_BAD;
            }
        }
        return LINE_OK;
    }

    LINE_STATUS parse_server(std::string &line) {
        istringstream iss(line);
        string word;
        while(iss >> word) {
            //TODO:如何确保正确接收到第一个左括号,保证server后面一定接受到{，而不是别的。
            if (server_left_bracket == false) {
                if (word == "{") {
                    server_left_bracket = true;
                }
                else {
                    throw std::runtime_error("Expected '{' after 'server' ");
                    return LINE_BAD;
                }
            }
            else if (word == "server_name") {
                iss >> this->servers.back().server_name;
            }
            else if (word == "listen") {
                iss >> this->servers.back().listen;
            }
            else if (word == "location") {
                check_state = LOCATION;
                Location loc = Location();
                iss >> loc.id;
                curLocation = loc.id;
                this->servers.back().locations.insert({curLocation, loc});
                parse_bracket(iss, location_left_bracket);
                break;
            }
            else if (word == "}") {
                server_left_bracket = false;
                check_state = BEGAIN;
            }
            else {
                return LINE_BAD;
            }
        }
        return LINE_OK;
    }

    LINE_STATUS parse_location(std::string &line) {
        istringstream iss(line);
        string word;
        while (iss >> word) {
            if (!location_left_bracket) {
                if (word == "{") {
                    location_left_bracket = true;
                } else {
                    throw std::runtime_error("Expected '{' after 'location' name");
                    return LINE_BAD;
                }
            }
            else if (word == "proxy_pass") {
                iss >> servers.back().locations[curLocation].proxy_pass;
            }
            else if (word == "}") {
                location_left_bracket = false;
                check_state = SERVER;
            }
            else {
                return LINE_BAD;
            }
        }
        return LINE_OK;
    }

    LINE_STATUS parse_upstream(std::string &line) {
        istringstream iss(line);
        string word;
        while (iss >> word)
        {
            if (!upstream_left_bracket)
            {
                if (word == "{")
                {
                    upstream_left_bracket= true;
                }
                else
                {
                    throw std::runtime_error("Expected '{' after 'upstream' name");
                    return LINE_BAD;
                }
            }
            else if (word == "server")
            {
                string ip_port;
                iss >> ip_port;
                size_t pos = ip_port.find_first_of(':');
                if (pos == string::npos) return LINE_BAD;
                string ip = ip_port.substr(0, pos);
                string port = ip_port.substr(pos+1);
                upstreams[curUpstream].servers.push_back({ip, port});
            }
            else if (word == "}")
            {
                upstream_left_bracket = false;
                check_state = BEGAIN;
            }
            else
            {
                return LINE_BAD;
            }
        }
        return LINE_OK;


    }

private:
    // 函数用于移除字符串两端的空格和制表符
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";
        size_t note = str.find_first_of("#;");
        if (note != std::string::npos) {
            return str.substr(first, note - first);
        }
        size_t last = str.find_last_not_of(" #\t\n\r");
        return str.substr(first, last - first + 1);
    }
};

