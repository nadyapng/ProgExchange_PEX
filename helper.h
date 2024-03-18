#ifndef HELPER_H
#define HELPER_H


int add_order_book(struct ORDER *order, struct ORDER **order_arr, int *index);

int amend_order(struct ORDER* order, struct ORDER** buy_arr, struct ORDER** sell_arr, int buy_arr_i, int sell_arr_i, struct TRADER** trader_arr, int num_traders, int trader_id, int order_arr_i, long *exchange_fees, int num_products);

int cancel_order(struct ORDER* order, struct ORDER** buy_arr, struct ORDER** sell_arr, int buy_arr_i, int sell_arr_i, struct TRADER** trader_arr, int num_traders, int trader_id);
void free_order_arr(struct ORDER** arr, int arr_i);
void free_trader_arr(struct TRADER** trader_arr, int num_traders, int num_products);
void free_product_arr(struct PRODUCT** product_arr, int num_products);
void free_arrays(struct ORDER** buy_arr, struct ORDER** sell_arr, struct TRADER** trader_arr, struct PRODUCT** product_arr, int buy_arr_i, int sell_arr_i, int num_traders, int num_products);
#endif