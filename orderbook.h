#ifndef ORDERBOOK_H
#define ORDERBOOK_H


void update_positions_sell(long *exchange_fees, long matching_price, int num_products, int qty, char* product_name, struct ORDER* old_order, struct ORDER* new_order, struct TRADER **trader_arr, int num_traders);
void update_positions_buy(long *exchange_fees, long matching_price, int num_products, int qty, char* product_name, struct ORDER* old_order, struct ORDER* new_order, struct TRADER **trader_arr, int num_traders);
void match_sell(struct ORDER* sell_order, struct ORDER** buy_arr, int buy_arr_i, long *exchange_fees, int num_products, struct TRADER** trader_arr, int num_traders);
void match_buy(struct ORDER* buy_order, struct ORDER** sell_arr, int sell_arr_i, long *exchange_fees, int num_products, struct TRADER** trader_arr, int num_traders);

int calc_levels(struct ORDER** arr, int arr_i, char* product_name);
void print_levels(struct ORDER** arr, int arr_i, char* product_name);
void print_positions(struct TRADER** trader_arr, int num_traders, int num_products);
void print_orderbook(struct ORDER** buy_arr, struct ORDER** sell_arr, struct PRODUCT** product_arr, int buy_arr_i, int sell_arr_i, int num_products);
#endif