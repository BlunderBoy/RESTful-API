#ifndef _CLIENT_HPP
#define _CLIENT_HPP 1

#include <string>

using namespace std;
class Client {
    public:
    string name;
    string password;
    bool is_looged_in;
    int current_descriptor;
    string curent_cookie;
    int requests_left;
};

#endif
