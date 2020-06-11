#ifndef _PRODUCT_HPP
#define _PRODUCT_HPP 1

#include <string>
#include <vector>
#include "client.hpp"

using namespace std;
class Product {
    public:
    int id;
    string name;
    double price;
    string category;
    string createdDate;
    string updatedDate;
};

#endif
