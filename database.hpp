#ifndef _DATABASE_HPP
#define _DATABASE_HPP 1

#include <string>
#include <vector>
#include "client.hpp"
#include "product.hpp"

using namespace std;
class Database {
    public:
    vector<Client> clients;
    vector<Product> products;
};

#endif
