#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"

#define LOG_PREFIX "[PEX]"

struct PRODUCT {
    char* name;
    int qty;
    long money;
};

enum ORDER_TYPE {
    BUY,
    SELL,
    AMEND,
    CANCEL,
    INVALID
};

struct TRADER {
    int id;
    int exchange_fd;
    int trader_fd;
    int pid;
    struct PRODUCT* product_info;
    int disconnected;
    int current_order_id;
};

struct ORDER {
    enum ORDER_TYPE type;
    int id;
    long price;
    int qty;
    char* product;
    struct TRADER trader;
    int time_i;
};


#endif