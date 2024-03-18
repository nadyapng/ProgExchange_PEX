#ifndef PARSE_H
#define PARSE_H


int check_product(struct PRODUCT** product_arr, int num_products, char* product_name);

int check_qty_price(long num);

int check_order_id(int order_id, struct ORDER* order, struct TRADER** trader_arr);

int check_digit(char* str);

struct ORDER parse_msg(char *msg, struct TRADER trader, int order_arr_i, struct ORDER** buy_arr, int buy_arr_i, struct PRODUCT** product_arr, int num_products, struct TRADER** trader_arr);

#endif