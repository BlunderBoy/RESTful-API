#ifndef _RATE_LIMIT_HPP
#define _RATE_LIMIT_HPP 1

#include <string>
#include <vector>
#include <utility>
#include "client.hpp"
#include "product.hpp"

using namespace std;

class User{
    public:
    uint32_t ip;
    long time_here;
    int request_limit;
    int descriptor;
};

class RateLimiter {
    public:
    vector<User> users;
};

#endif
