#ifndef PE_TRADER_H
#define PE_TRADER_H

#include "pe_common.h"

struct ORDER_T {
    char* type;
    int id;
    int price;
    int qty;
    char* product;
    // array for postions
    struct PRODUCT* positions;
};

#endif