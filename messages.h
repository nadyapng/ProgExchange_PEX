#ifndef MESSAGES_H
#define MESSAGES_H


void send_market_open(struct TRADER** trader_arr, int num_traders);
void send_fill(struct ORDER *order, int qty, struct TRADER** trader_arr);
void send_invalid(struct ORDER* order);
void send_accepted(struct ORDER *order, struct TRADER** trader_arr);
void broadcast_market(struct ORDER* order, struct TRADER** trader_arr, int num_traders);
void send_cancelled(struct ORDER* order, struct TRADER** trader_arr);
void send_amend(struct ORDER* order, struct TRADER** trader_arr);

#endif