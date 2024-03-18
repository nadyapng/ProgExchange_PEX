/**
 * comp2017 - assignment 3
 * Nadya Png Ee
 * npng6746
 */

#include "pe_exchange.h"
#include "pe_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include<sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>
#include <math.h>

#include "signals.h"
#include "fifo.h"
#include "parse.h"
#include "helper.h"
#include "matching.h"
#include "messages.h"
#include "orderbook.h"

#define PRODUCT_LEN 17
#define BUFFER 128

int sig_pid;
int signal_received;
int sigchld_pid;
int sigchld_received;


void get_fifo_exchange(int trader_id, char *name) {
    sprintf(name, FIFO_EXCHANGE, trader_id);
}

void get_fifo_trader(int trader_id, char *name) {
    sprintf(name, FIFO_TRADER, trader_id);
}

// sig handler
void handle_sigusr(int sig, siginfo_t *info, void *ucontext) {
    // get pid
	sig_pid = info->si_pid;
	signal_received = 1;
	
}

void handle_sigchld(int sig, siginfo_t *info, void *ucontext) {
	sigchld_received = 1;
	sigchld_pid = info->si_pid;
}

char* enum_str(enum ORDER_TYPE type) {
	switch(type) {
		case BUY:
			return "BUY";
		case SELL:
			return "SELL";
		case AMEND:
			return "AMEND";
		case CANCEL:
			return "CANCEL";
		case INVALID:
			return "INVALID";

	}
	return NULL;
}

int check_product(struct PRODUCT** product_arr, int num_products, char* product_name) {
	for (int i = 0 ; i < num_products ; i++) {
		if(strcmp(product_name, (*product_arr)[i].name) == 0) {
			return 0;
		}
	}
	return 1;
}

int check_qty_price(long num) {
	// check negaative and check out of range
	if ((num < 1) || (num > 999999)) {
		return 1;
	}
	return 0;
}

int check_order_id(int order_id, struct ORDER* order, struct TRADER** trader_arr) {
	// check if incremental
	if (order_id != ((*trader_arr)[order->trader.id].current_order_id+1)) {
		return 1;
	}
	else if ((order_id < 0) || (order_id > 999999)) {
		return 1;
	}
	return 0;
}

int check_digit(char* str) {
    for (int i = 0 ; i <strlen(str) ; i++) {
        if(isdigit(str[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

struct ORDER parse_msg(char *msg, struct TRADER trader, int order_arr_i, struct ORDER** buy_arr, int buy_arr_i, struct PRODUCT** product_arr, int num_products, struct TRADER** trader_arr) {
	struct ORDER order;
    // strip end ;
    msg[strcspn(msg, ";")] = 0;

	// print message
	printf("[PEX] [T%d] Parsing command: <%s>\n", trader.id, msg);
	// set trader
	order.trader = trader;
    // split command
    char *ptr = strtok(msg, " ");
	if (ptr == NULL) {
		order.type = INVALID;
		return order;
	}
	
    if ((strcmp(ptr, "BUY") == 0)) {
        order.type = BUY;
		char* temp_id = strtok(NULL, " ");
		
        char* temp_product = strtok(NULL, " ");
		
		char* temp_qty = strtok(NULL, " ");
        
		char* temp_price = strtok(NULL, " ");

		// check if field not given
		if((temp_id == NULL) || (temp_product == NULL) || (temp_qty == NULL) || (temp_price == NULL)) {
			order.type = INVALID;
			return order;
		}

		// check if non integer given
		if((check_digit(temp_id) == 1) || (check_digit(temp_qty) == 1) || (check_digit(temp_price) == 1)) {
			order.type = INVALID;
			return order;
		}

		order.id = atoi(temp_id);
		if (check_order_id(order.id, &order, trader_arr) == 1) {
			order.type = INVALID;
			return order;
		}

		// initialise string
		order.product = malloc(strlen(temp_product)+1);

		if (order.product == NULL) {
			printf("Error allocating for product name\n");
			exit(1);
		}

		strcpy(order.product, temp_product);
		if (check_product(product_arr, num_products, order.product) == 1) {
			order.type = INVALID;
			free(order.product);
			return order;
		}
		order.qty = atoi(temp_qty);
        order.price = atoi(temp_price);
		// check qty and price within range
		if ((check_qty_price(order.qty) == 1) || (check_qty_price(order.price) == 1)) {
			order.type = INVALID;
			free(order.product);
			return order;
		}


		order.time_i = order_arr_i;
		
    }
	else if (strcmp(ptr, "SELL") == 0) {
        order.type = SELL;
		char* temp_id = strtok(NULL, " ");
		
        char* temp_product = strtok(NULL, " ");
		
		char* temp_qty = strtok(NULL, " ");
        
		char* temp_price = strtok(NULL, " ");

		if((temp_id == NULL) || (temp_product == NULL) || (temp_qty == NULL) || (temp_price == NULL)) {
			order.type = INVALID;
			return order;
		}

		// check if non integer given
		if((check_digit(temp_id) == 1) || (check_digit(temp_qty) == 1) || (check_digit(temp_price) == 1)) {
			order.type = INVALID;
			return order;
		}

		order.id = atoi(temp_id);
		if (check_order_id(order.id, &order, trader_arr) == 1) {
			order.type = INVALID;
			return order;
		}

		order.product = malloc(strlen(temp_product)+1);

		if (order.product == NULL) {
			printf("Error allocating for product name\n");
			exit(1);
		}

		strcpy(order.product, temp_product);
		if (check_product(product_arr, num_products, order.product) == 1) {
			order.type = INVALID;
			free(order.product);
			return order;
		}
		order.qty = atoi(temp_qty);
        order.price = atoi(temp_price);
		if ((check_qty_price(order.qty) == 1) || (check_qty_price(order.price) == 1)) {
			order.type = INVALID;
			free(order.product);
			return order;
		}
		order.time_i = order_arr_i;
    }
    else if ((strcmp(ptr, "AMEND") == 0)){
		order.type = AMEND;
		char* temp_id = strtok(NULL, " ");
		char* temp_qty = strtok(NULL, " ");
		char* temp_price = strtok(NULL, " ");

		if ((temp_id == NULL) || (temp_qty == NULL) || (temp_price == NULL)) {
			order.type = INVALID;
			return order;
		}

		// check if non integer given
		if((check_digit(temp_id) == 1) || (check_digit(temp_qty) == 1) || (check_digit(temp_price) == 1)) {
			order.type = INVALID;
			return order;
		}

		order.id = atoi(temp_id);
		order.qty = atoi(temp_qty);
        order.price = atoi(temp_price);
		if ((check_qty_price(order.qty) == 1) || (check_qty_price(order.price) == 1)) {
			order.type = INVALID;
			return order;
		}
    }
    else if (strcmp(ptr, "CANCEL") == 0) {
        order.type = CANCEL;
		char* temp_id = strtok(NULL, " ");
		if (temp_id == NULL) {
			order.type = INVALID;
			return order;
		}
		// check if non integer given
		if((check_digit(temp_id) == 1)) {
			order.type = INVALID;
			return order;
		}
		order.id = atoi(temp_id);
    }
	else {
		order.type = INVALID;
	}

    return order;
}

int add_order_book(struct ORDER *order, struct ORDER **order_arr, int *index) {
	if((*order_arr = realloc(*order_arr, (*index+1)*sizeof(struct ORDER))) == NULL) {
		printf("Failed to realloc\n");
		return 1;
	}
	(*order_arr)[*index] = *order;
	(*index)++;
	return 0;
}

void send_market_open(struct TRADER** trader_arr, int num_traders) {
	struct TRADER* temp_trader_arr = *trader_arr;
	for(int i = 0 ; i < num_traders ; i++) {
		write(temp_trader_arr[i].exchange_fd, "MARKET OPEN;", strlen("MARKET OPEN;"));
		kill(temp_trader_arr[i].pid, SIGUSR1);
	}
}

void send_fill(struct ORDER *order, int qty, struct TRADER** trader_arr) {
	char* new_msg = (char*) malloc(strlen("FILL")+1+6+6+1+1); 
	if(new_msg == NULL) {
		printf("Error allocating for new message\n");
		exit(1);
	}
	sprintf(new_msg, "FILL %d %d;", order->id, qty);
	if((*trader_arr)[order->trader.id].disconnected != 1) {
		write(order->trader.exchange_fd, new_msg, strlen(new_msg));
		kill(order->trader.pid, SIGUSR1);
	}
	free(new_msg);
	
}

void send_invalid(struct ORDER* order) {
	write(order->trader.exchange_fd, "INVALID;", strlen("INVALID;"));
	kill(order->trader.pid, SIGUSR1);
}

void send_accepted(struct ORDER *order, struct TRADER** trader_arr) {
	char* new_msg = (char*) malloc(strlen("ACCEPTED")+1+6+1); // max for id is 6 chars and 1 spaces + ;
	if(new_msg == NULL) {
		printf("Error allocating for new message\n");
		exit(1);
	}
	if((*trader_arr)[order->trader.id].disconnected != 1) {
		sprintf(new_msg, "ACCEPTED %d;", order->id);
		write(order->trader.exchange_fd, new_msg, strlen(new_msg));
		kill(order->trader.pid, SIGUSR1);
	}
	free(new_msg);
}

// sort price high to low
int comparator_buy(const void *p, const void *q) {
    long price1 = ((struct ORDER*) p)->price;
    long price2 = ((struct ORDER*) q)->price; 
	long time1 = ((struct ORDER*) p)->time_i;
	long time2 = ((struct ORDER*) q)->time_i;
    
	if(price1 < price2) {
		return 1;
	}
	else if (price1 == price2) {
		if(time1 > time2) {
			return 1;
		}
		return -1;
	}
	return -1;
}

// sort price low to high
int comparator_sell(const void *p, const void *q) {
    long price1 = ((struct ORDER*) p)->price;
    long price2 = ((struct ORDER*) q)->price; 
	long time1 = ((struct ORDER*) p)->time_i;
	long time2 = ((struct ORDER*) q)->time_i;
    
	if(price1 > price2) {
		return 1;
	}
	else if (price1 == price2) {
		if(time1 > time2) {
			return 1;
		}
		return -1;
	}
	return -1;
}

void broadcast_market(struct ORDER* order, struct TRADER** trader_arr, int num_traders) {
	// generate message
	char* new_msg = (char*) malloc(strlen("MARKET")+strlen(enum_str(order->type))+PRODUCT_LEN+6+6+5); // max for qty and price is 6 chars and 4 spaces + ;
	if(new_msg == NULL) {
		printf("Error allocating for new message\n");
		exit(1);
	}
	sprintf(new_msg, "MARKET %s %s %d %ld;", enum_str(order->type), order->product, order->qty, order->price);
	struct TRADER* temp_trader_arr = *trader_arr;
	// broadcast
	for(int i = 0 ; i < num_traders ; i++) {
		struct TRADER current_trader = temp_trader_arr[i];
		if (current_trader.disconnected == 1) {
			continue;
		}
		// check if sending trader
		if((order->trader).pid == current_trader.pid) {continue;}
		write(current_trader.exchange_fd, new_msg, strlen(new_msg));
		kill(current_trader.pid, SIGUSR1);
	}
	free(new_msg);
} 

void update_positions_sell(long *exchange_fees, long matching_price, int num_products, int qty, char* product_name, struct ORDER* old_order, struct ORDER* new_order, struct TRADER **trader_arr, int num_traders) {
	long fee = round(0.01 * (matching_price*qty));
	*exchange_fees += fee;
	long seller_revenue = round((matching_price*qty)-fee);

	struct TRADER* temp_trader_arr = *trader_arr;

	int index = new_order->trader.id;

	// cache
	int product_index = 0;

	for (int j = 0 ; j < num_products ; j++) {
		struct PRODUCT current_product = temp_trader_arr[index].product_info[j];
		if (strcmp(current_product.name, product_name) == 0) {
			product_index = j;
			// update money
			temp_trader_arr[index].product_info[j].money += seller_revenue;
			// update qty
			temp_trader_arr[index].product_info[j].qty -= qty;
			break;
		}
		
	}

	long buyer_owe = matching_price * qty;

	index = old_order->trader.id;

	temp_trader_arr[index].product_info[product_index].money -= buyer_owe;
	// update qty
	temp_trader_arr[index].product_info[product_index].qty += qty;

	printf("[PEX] Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", old_order->id, old_order->trader.id, new_order->id, new_order->trader.id, buyer_owe, fee);
		
}


void update_positions_buy(long *exchange_fees, long matching_price, int num_products, int qty, char* product_name, struct ORDER* old_order, struct ORDER* new_order, struct TRADER **trader_arr, int num_traders) {
	long fee = round(0.01 * (matching_price*qty));
	*exchange_fees += fee;
	long seller_revenue = (matching_price*qty);
	int index = old_order->trader.id;

	struct TRADER* temp_trader_arr = *trader_arr;

	// cache
	int product_index = 0;

	// find product struct
	for (int j = 0 ; j < num_products ; j++) {
		struct PRODUCT current_product = temp_trader_arr[index].product_info[j];
		if (strcmp(current_product.name, product_name) == 0) {
			product_index = j;
			// update money
			temp_trader_arr[index].product_info[j].money += round(seller_revenue);
			// update qty
			temp_trader_arr[index].product_info[j].qty -= qty;
			break;
		}
		
	}

	long buyer_owe = round((matching_price * qty) + fee);

	index = new_order->trader.id;

	temp_trader_arr[index].product_info[product_index].money -= buyer_owe;
	// update qty
	temp_trader_arr[index].product_info[product_index].qty += qty;
			
	
	printf("[PEX] Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", old_order->id, old_order->trader.id, new_order->id, new_order->trader.id, seller_revenue, fee);
	
}

// MUST PASS sell_order as an order in array
void match_sell(struct ORDER* sell_order, struct ORDER** buy_arr, int buy_arr_i, long *exchange_fees, int num_products, struct TRADER** trader_arr, int num_traders) {
	struct ORDER* temp_buy_arr = *buy_arr;

	int i = 0;
	while((sell_order->qty > 0) && (i < buy_arr_i)) {
		struct ORDER current_order = temp_buy_arr[i];

		// skip if sold/cancelled order
		if (current_order.type == INVALID) {
			i++; 
			continue;
		}
		// skip if not the same product
		else if (strcmp(sell_order->product, current_order.product) != 0) {
			i++; 
			continue;
		}
		// check price
		else if (sell_order->price <= current_order.price) {
			// match
			long matching_price = current_order.price;
			if (sell_order->qty > current_order.qty) {
				// more in sell order than buy order -> balance in sell
				// send fill 
				send_fill(&(temp_buy_arr[i]), current_order.qty, trader_arr);
				send_fill(sell_order, current_order.qty, trader_arr);
				// dedcut qty
				update_positions_sell(exchange_fees, matching_price, num_products, current_order.qty, sell_order->product, &(temp_buy_arr[i]), sell_order, trader_arr, num_traders);
				
				sell_order->qty -= temp_buy_arr[i].qty;
				temp_buy_arr[i].qty = 0;
				temp_buy_arr[i].type = INVALID;
			}
			else if (sell_order->qty < current_order.qty) {
				// more in buy order than sell order -> balance in buy
				// send fill 
				send_fill(&(temp_buy_arr[i]), sell_order->qty, trader_arr);
				send_fill(sell_order, sell_order->qty, trader_arr);

				update_positions_sell(exchange_fees, matching_price, num_products, sell_order->qty, sell_order->product, &(temp_buy_arr[i]), sell_order, trader_arr, num_traders);
				
				// dedcut qty
				temp_buy_arr[i].qty -= sell_order->qty;
				sell_order->qty = 0;
				sell_order->type = INVALID;
				break;
			}
			else if (sell_order->qty == current_order.qty) {
				// qty in sell order = qty in buy order
				// send fill 
				send_fill(&(temp_buy_arr[i]), current_order.qty, trader_arr);
				send_fill(sell_order, temp_buy_arr[i].qty, trader_arr);

				update_positions_sell(exchange_fees, matching_price, num_products, current_order.qty, sell_order->product, &(temp_buy_arr[i]), sell_order, trader_arr, num_traders);
				
				// dedcut qty
				temp_buy_arr[i].qty = 0;
				sell_order->qty = 0;
				temp_buy_arr[i].type = INVALID;
				sell_order->type = INVALID;
				
				break;
			}

		}
		
		i++;
	}
}

// MUST PASS buy_order as an order in array
void match_buy(struct ORDER* buy_order, struct ORDER** sell_arr, int sell_arr_i, long *exchange_fees, int num_products, struct TRADER** trader_arr, int num_traders) {
	struct ORDER* temp_sell_arr = *sell_arr;
	
	int i = 0;
	while((buy_order->qty > 0) && (i < sell_arr_i)) {
		struct ORDER current_order = temp_sell_arr[i];
		// skip if sold/cancelled order
		if (current_order.type == INVALID) {
			i++; 
			continue;
		}
		// skip if not the same product
		else if (strcmp(buy_order->product, current_order.product) != 0) {
			i++; 
			continue;
		}
		// check price
		else if (buy_order->price >= current_order.price) {
			long matching_price = current_order.price;
			// match
			if (buy_order->qty > current_order.qty) {
				// more in sell order than buy order -> balance in sell
				// send fill 
				send_fill(buy_order, current_order.qty, trader_arr);
				send_fill(&(temp_sell_arr[i]), current_order.qty, trader_arr);

				update_positions_buy(exchange_fees, matching_price, num_products, current_order.qty, buy_order->product, &temp_sell_arr[i], buy_order, trader_arr, num_traders);
				// dedcut qty
				buy_order->qty -= temp_sell_arr[i].qty;
				temp_sell_arr[i].qty = 0;
				temp_sell_arr[i].type = INVALID;

			}
			else if (buy_order->qty < current_order.qty) {
				// more in buy order than sell order -> balance in buy
				// send fill 
				send_fill(buy_order, buy_order->qty, trader_arr);
				send_fill(&(temp_sell_arr[i]), buy_order->qty, trader_arr);

				update_positions_buy(exchange_fees, matching_price, num_products, buy_order->qty, buy_order->product, &temp_sell_arr[i], buy_order, trader_arr, num_traders);
				// dedcut qty
				temp_sell_arr[i].qty -= buy_order->qty;
				buy_order->qty = 0;
				buy_order->type = INVALID;
				break;
			}
			else if (buy_order->qty == current_order.qty) {
				// qty in sell order = qty in buy order
				// send fill 
				send_fill(buy_order, current_order.qty, trader_arr);
				send_fill(&(temp_sell_arr[i]), current_order.qty, trader_arr);

				update_positions_buy(exchange_fees, matching_price, num_products, current_order.qty, buy_order->product, &temp_sell_arr[i], buy_order, trader_arr, num_traders);
				// dedcut qty
				temp_sell_arr[i].qty = 0;
				buy_order->qty = 0;
				temp_sell_arr[i].type = INVALID;
				buy_order->type = INVALID;
				
				break;
			}

		}
		
		i++;
	}
}


void send_cancelled(struct ORDER* order, struct TRADER** trader_arr) {
	char* new_msg = (char*) malloc(strlen("CANCELLED")+1+6+1); // max for id is 6 chars and 1 spaces + ;
	if(new_msg == NULL) {
		printf("Error allocating for new message\n");
		exit(1);
	}
	sprintf(new_msg, "CANCELLED %d;", order->id);
	if ((*trader_arr)[order->trader.id].disconnected != 1) {
		write(order->trader.exchange_fd, new_msg, strlen(new_msg));
		kill(order->trader.pid, SIGUSR1);
	}
	free(new_msg);
}

void send_amend(struct ORDER* order, struct TRADER** trader_arr) {
	char* new_msg = (char*) malloc(strlen("AMENDED")+1+6+1); // max for id is 6 chars and 1 spaces + ;
	if(new_msg == NULL) {
		printf("Error allocating for new message\n");
		exit(1);
	}
	sprintf(new_msg, "AMENDED %d;", order->id);
	if ((*trader_arr)[order->trader.id].disconnected != 1) {
		write(order->trader.exchange_fd, new_msg, strlen(new_msg));
		kill(order->trader.pid, SIGUSR1);
	}
	free(new_msg);
}

int amend_order(struct ORDER* order, struct ORDER** buy_arr, struct ORDER** sell_arr, int buy_arr_i, int sell_arr_i, struct TRADER** trader_arr, int num_traders, int trader_id, int order_arr_i, long *exchange_fees, int num_products) {
	// search buy arr
	// check order id and trader
	struct ORDER* temp_buy_arr = *buy_arr;
	struct ORDER* temp_sell_arr = *sell_arr;

	for (int i  = 0 ; i < buy_arr_i ; i++) {
		struct ORDER current_order = temp_buy_arr[i];
		if ((trader_id == current_order.trader.id) && (order->id == current_order.id)) {
			if (current_order.type == INVALID) {
				return 1;
			}
			// send amend
			send_amend(order, trader_arr);
			// set values
			temp_buy_arr[i].qty = order->qty;
			temp_buy_arr[i].price = order->price;
			temp_buy_arr[i].time_i = order_arr_i;
			// broadcast
			broadcast_market(&temp_buy_arr[i], trader_arr, num_traders);

			// match
			match_buy(&((*buy_arr)[i]), sell_arr, sell_arr_i, exchange_fees, num_products, trader_arr, num_traders);
			return 0;
		}
	}
	// check sell arr
	for (int i  = 0 ; i < sell_arr_i ; i++) {
		struct ORDER current_order = temp_sell_arr[i];
		if ((trader_id == (current_order.trader).id) && (order->id == current_order.id)) {
			if (current_order.type == INVALID) {
				return 1;
			}
			// send amend
			send_amend(order, trader_arr);
			// set values
			temp_sell_arr[i].qty = order->qty;
			temp_sell_arr[i].price = order->price;
			temp_sell_arr[i].time_i = order_arr_i;
			// broadcast
			broadcast_market(&temp_sell_arr[i], trader_arr, num_traders);

			match_sell(&((*sell_arr)[i]), buy_arr, buy_arr_i, exchange_fees, num_products, trader_arr, num_traders);
			return 0;
		}
	}
	// cant find order
	return 1;
}


int cancel_order(struct ORDER* order, struct ORDER** buy_arr, struct ORDER** sell_arr, int buy_arr_i, int sell_arr_i, struct TRADER** trader_arr, int num_traders, int trader_id) {
	// search buy arr
	// check order id and trader
	struct ORDER* temp_buy_arr = *buy_arr;
	struct ORDER* temp_sell_arr = *sell_arr;

	for (int i  = 0 ; i < buy_arr_i ; i++) {
		struct ORDER current_order = temp_buy_arr[i];
		if ((trader_id == current_order.trader.id) && (order->id == current_order.id)) {
			if (current_order.type == INVALID) {
				return 1;
			}
			// send cancelled
			send_cancelled(order, trader_arr);
			// set values
			temp_buy_arr[i].qty = 0;
			temp_buy_arr[i].price = 0;
			// broadcast
			broadcast_market(&temp_buy_arr[i], trader_arr, num_traders);
			temp_buy_arr[i].type = INVALID;
			return 0;
		}
	}
	// check sell arr
	for (int i  = 0 ; i < sell_arr_i ; i++) {
		struct ORDER current_order = temp_sell_arr[i];
		if ((trader_id == (current_order.trader).id) && (order->id == current_order.id)) {
			if (current_order.type == INVALID) {
				return 1;
			}
			// send cancelled
			send_cancelled(order, trader_arr);
			// set values
			temp_sell_arr[i].qty = 0;
			temp_sell_arr[i].price = 0;
			// broadcast
			broadcast_market(&temp_sell_arr[i], trader_arr, num_traders);
			temp_sell_arr[i].type = INVALID;
			return 0;
		}
	}
	// cant find order
	return 1;
}

int calc_levels(struct ORDER** arr, int arr_i, char* product_name) {
	// array MUST be sorted
	long last_price = -1;
	int level = 0;

	struct ORDER* temp_arr = *arr;
	for(int i = 0 ; i < arr_i ; i++) {
		struct ORDER current_order = temp_arr[i];
		// check if sold or cancelled
		if((current_order.type == INVALID) || (strcmp(current_order.product, product_name) != 0)) {
			continue;
		}
		// if not same as last price then is new price level
		if(current_order.price != last_price) {
			level++;
			last_price = current_order.price;
		}
	}
	return level;
}

void print_levels(struct ORDER** arr, int arr_i, char* product_name) {
	long last_price = -1;
	int order_count = 0;
	int qty_count = 0;
	enum ORDER_TYPE last_type;

	struct ORDER* temp_arr = *arr;
	for(int i = 0 ; i < arr_i ; i++) {
		struct ORDER current_order = temp_arr[i];

		// check if sold or cancelled
		if((current_order.type == INVALID) || (strcmp(current_order.product, product_name) != 0)) {
			continue;
		}
		if(current_order.price != last_price) {
			// print last price and quant
			if((i > 0) && (last_price != -1)) {
				if(order_count > 1) {
					printf("[PEX]\t\t%s %d @ $%ld (%d orders)\n", enum_str(last_type), qty_count, last_price, order_count);
				}
				else {
					printf("[PEX]\t\t%s %d @ $%ld (%d order)\n", enum_str(last_type), qty_count, last_price, order_count);
				}
			}

			// reset values
			last_type = current_order.type;
			order_count = 1;
			qty_count = current_order.qty;
			last_price = current_order.price;
		}
		else {
			order_count++;
			qty_count += current_order.qty;
		}
	}
	if((order_count > 1) && (last_price != -1)) {
		printf("[PEX]\t\t%s %d @ $%ld (%d orders)\n", enum_str(last_type), qty_count, last_price, order_count);
	}
	else if (last_price != -1) {
		printf("[PEX]\t\t%s %d @ $%ld (%d order)\n", enum_str(last_type), qty_count, last_price, order_count);
	}
}

void print_positions(struct TRADER** trader_arr, int num_traders, int num_products) {
	printf("[PEX]\t--POSITIONS--\n");

	struct TRADER* temp_trader_arr = *trader_arr;
	for (int i = 0 ; i < num_traders ; i++) {
		struct TRADER current_trader = temp_trader_arr[i];
		printf("[PEX]\tTrader %d:", current_trader.id);
		for (int j = 0 ; j < num_products ; j++) { 
			struct PRODUCT current_product = current_trader.product_info[j];
			if (j == (num_products-1)) {
				printf(" %s %d ($%ld)\n", current_product.name, current_product.qty, current_product.money);
			}
			else {
				printf(" %s %d ($%ld),", current_product.name, current_product.qty, current_product.money);
			}
		}
	}
}

void print_orderbook(struct ORDER** buy_arr, struct ORDER** sell_arr, struct PRODUCT** product_arr, int buy_arr_i, int sell_arr_i, int num_products) {
	// get combined array
	struct ORDER* all_order_arr = (struct ORDER*) malloc((buy_arr_i+sell_arr_i)*sizeof(struct ORDER));
	if(all_order_arr == NULL) {
		printf("Error allocating for order array\n");
		exit(1);
	}

	// add every order to array
	struct ORDER* temp_buy_arr = *buy_arr;
	struct ORDER* temp_sell_arr = *sell_arr;

	for (int i = 0 ; i < buy_arr_i ; i++) {
		all_order_arr[i] = temp_buy_arr[i];
	}
	for (int i = buy_arr_i ; i < (buy_arr_i+sell_arr_i) ; i++) {
		all_order_arr[i] = temp_sell_arr[i-buy_arr_i];
	}
	// sort combined array
	qsort((void*) all_order_arr, (buy_arr_i+sell_arr_i), sizeof(struct ORDER), comparator_buy);


	struct PRODUCT* temp_product_arr = *product_arr;
	printf("[PEX]\t--ORDERBOOK--\n");
	for(int i = 0 ; i < num_products ; i++) {
		struct PRODUCT currrent_product = temp_product_arr[i];
		int buy_levels = calc_levels(buy_arr, buy_arr_i, currrent_product.name);
		int sell_levels = calc_levels(sell_arr, sell_arr_i, currrent_product.name);
		printf("[PEX]\tProduct: %s; Buy levels: %d; Sell levels: %d\n", currrent_product.name, buy_levels, sell_levels);
		print_levels(&all_order_arr, (buy_arr_i+sell_arr_i), currrent_product.name);
	}

	
	free(all_order_arr);
}

int check_disconnect(struct TRADER** trader_arr, int num_traders, int *num_disconnected, long *exchange_fees) {
	if (sigchld_received == 1) {
		// find disconnected trader
		struct TRADER* temp_trader_arr = (*trader_arr);
		for(int i = 0 ; i < num_traders ; i++) {
			if (sigchld_pid == temp_trader_arr[i].pid) {
				printf("[PEX] Trader %d disconnected\n", temp_trader_arr[i].id);
				temp_trader_arr[i].disconnected = 1;
				(*num_disconnected)++;
			}
		}

		// check if all disconnected
		if (*num_disconnected == num_traders) {
			printf("[PEX] Trading completed\n");
			printf("[PEX] Exchange fees collected: $%ld\n", *exchange_fees);
			return 1;
		}

		// reset flag
		sigchld_received = 0;
		return 2;
	}
	return 0;
}

void free_order_arr(struct ORDER** arr, int arr_i) {
	for (int i = 0 ; i < arr_i ; i++) {
		free((*arr)[i].product);
	}
	free(*arr);
}

void free_trader_arr(struct TRADER** trader_arr, int num_traders, int num_products) {
	for (int i = 0; i < num_traders ; i++) {
		for (int j = 0 ; j < num_products ; j++) {
			free((*trader_arr)[i].product_info[j].name);
		}
		free((*trader_arr)[i].product_info);
	}
	free(*trader_arr);
}

void free_product_arr(struct PRODUCT** product_arr, int num_products) {
	for (int i = 0 ; i < num_products ; i++) {
		free((*product_arr)[i].name);
	}
	free(*product_arr);
}

void free_arrays(struct ORDER** buy_arr, struct ORDER** sell_arr, struct TRADER** trader_arr, struct PRODUCT** product_arr, int buy_arr_i, int sell_arr_i, int num_traders, int num_products) {
	// free buy array
	// free product names
	free_order_arr(buy_arr, buy_arr_i);

	// free sell array
	// free product names
	free_order_arr(sell_arr, sell_arr_i);

	// free trader array
	free_trader_arr(trader_arr, num_traders, num_products);

	// free product array
	free_product_arr(product_arr, num_products);

}


int main(int argc, char **argv) {
	if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }

	printf("[PEX] Starting\n");

	// read product file
	FILE *product_file = fopen(argv[1], "r");
	//fopen error handling
	if(product_file == NULL) {
		printf("Cannot open product file\n");
		return 1;
	}

	int num_products; 


	char* buffer;
	buffer = (char*) malloc(PRODUCT_LEN*sizeof(char));
	if(buffer == NULL) {
		printf("Error allocating for buffer\n");
		return 1;
	}
	size_t max_len = PRODUCT_LEN;

	// get number
	getline(&buffer, &max_len, product_file);
	buffer[strlen(buffer)-1] = '\0';
	num_products = atoi(buffer);

	// array to save all products
	struct PRODUCT* product_arr = (struct PRODUCT*) malloc(num_products*sizeof(struct PRODUCT));
	if(buffer == NULL) {
		printf("Error allocating for product array\n");
		free(buffer);
		return 1;
	}

	// print
	printf("[PEX] Trading %d products:", num_products);
	int product_arr_i = 0;
	while(getline(&buffer, &max_len, product_file) > 0) {
		// check for too many products
		if(product_arr_i >= num_products) {
			printf("Too many products!\n");
			free(buffer);
			free_product_arr(&product_arr, num_products);
			return 1;
		}

		buffer[strlen(buffer)-1] = '\0';
		// check product length 
		if(strlen(buffer) >= PRODUCT_LEN) {
			printf("Product length is too long\n");
			free(buffer);
			free_product_arr(&product_arr, num_products);
		}

		// allocate for each product name
		product_arr[product_arr_i].name = malloc(strlen(buffer)+1);
		if(product_arr[product_arr_i].name == NULL) {
			printf("Error allocating for product name\n");
			free(buffer);
			free_product_arr(&product_arr, num_products);
			return 1;
		}

		// copy name from buffer
		strcpy(product_arr[product_arr_i].name, buffer);
		printf(" %s", product_arr[product_arr_i].name);
		product_arr_i++;

	}
	printf("\n");

	// check number of actual products
	if(product_arr_i != num_products) {
		printf("Wrong number of products given in product file\n");
		free_product_arr(&product_arr, num_products);
		free(buffer);
	}
	free(buffer);


	// register signal handler
	// SIGCHLD
	struct sigaction sa_chld;
	sa_chld.sa_flags = SA_SIGINFO;
    sa_chld.sa_sigaction = &handle_sigchld;
    sigaction(SIGCHLD, &sa_chld, NULL);
	
	// SIGUSR1
    struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &handle_sigusr;
    sigaction(SIGUSR1, &sa, NULL);


	// find number of trader_arr
	int num_traders = argc - 2;
	struct TRADER* trader_arr = (struct TRADER*) calloc(num_traders, sizeof(struct TRADER));
	if (trader_arr == NULL) {
		printf("Cannot calloc for trader_arr\n");
		free_product_arr(&product_arr, num_products);
		return 1;
	}
	

	// connect to trader_arr and add trader to trader array
	for (int i = 0 ; i < num_traders ; i++) {
		char* exchange_name = (char*) malloc(sizeof("/tmp/pe_exchange_") + sizeof(int));
		if(exchange_name == NULL) {
			printf("Error allocating for exchange name\n");
			free_trader_arr(&trader_arr, num_traders, num_products);
			free_product_arr(&product_arr, num_products);
			return 1;
		}

		get_fifo_exchange(i, exchange_name);
		if(mkfifo(exchange_name, 0777) != 0) {
			printf("Error making fifo\n");
			// free everything before exit
			free_product_arr(&product_arr, num_products);
			free(exchange_name);
			free_trader_arr(&trader_arr, num_traders, num_products);

			return 1;
		}
		printf("[PEX] Created FIFO %s\n", exchange_name);

		char* trader_name = (char*) malloc(sizeof("/tmp/pe_trader_") + sizeof(int));
		if(trader_name == NULL) {
			printf("Error allocating for trader name\n");
			free_trader_arr(&trader_arr, num_traders, num_products);
			free_product_arr(&product_arr, num_products);
			free(exchange_name);
			return 1;
		}

		get_fifo_trader(i, trader_name);
		if(mkfifo(trader_name, 0777) != 0) {
			printf("Error making fifo\n");
			// free everything before exit
			free_product_arr(&product_arr, num_products);
			free(exchange_name);
			free(trader_name);
			free_trader_arr(&trader_arr, num_traders, num_products);
			return 1;
		}
		printf("[PEX] Created FIFO %s\n", trader_name);

		int pid = fork();
		if(pid > 0) {
			// parent process
			// open fifo
			int fd_exchange = open(exchange_name, O_WRONLY);
			int fd_trader = open(trader_name, O_RDONLY);
			// error handling for opening
			if((fd_exchange == 0) || (fd_trader == 0)) {
				printf("Error opening fifo\n");
				free_product_arr(&product_arr, num_products);
				free(exchange_name);
				free(trader_name);
				free_trader_arr(&trader_arr, num_traders, num_products);
				return 1;
			}

			// add to trader_arr array
			struct TRADER new_trader;
			new_trader.exchange_fd = fd_exchange;
			new_trader.trader_fd = fd_trader;
			new_trader.pid = pid;
			new_trader.id = i;
			new_trader.product_info = (struct PRODUCT*) malloc(num_products*sizeof(struct PRODUCT));
			new_trader.disconnected = 0;
			new_trader.current_order_id = -1;
			for (int j = 0 ; j < num_products ; j++) {
				new_trader.product_info[j].name = (char*) malloc(strlen(product_arr[j].name)+1);
				strcpy(new_trader.product_info[j].name, product_arr[j].name);
				new_trader.product_info[j].money = 0;
				new_trader.product_info[j].qty = 0;
			}
			trader_arr[i] = new_trader;
		}
		else {
			// child process
			// run binary (argument is trader id)
			char id[11];
			sprintf(id, "%d", i);
			printf("[PEX] Starting trader %d (%s)\n", i, argv[i+2]);
			execl(argv[i+2], argv[i+2], id, NULL);
		}

		printf("[PEX] Connected to %s\n", exchange_name);
		printf("[PEX] Connected to %s\n", trader_name);

		free(exchange_name);
		free(trader_name);
	}

	// send market open and signal
	send_market_open(&trader_arr, num_traders);

	// all orders index
	int order_arr_i = 0;

	// array for buy orders
	struct ORDER *buy_arr = (struct ORDER*) (malloc(sizeof(struct ORDER)));
	int buy_arr_i = 0;

	// array for sell orders
	struct ORDER *sell_arr = (struct ORDER*) (malloc(sizeof(struct ORDER)));
	int sell_arr_i = 0;

	// exchange transaction fees
	long exchange_fees = 0;

	// count disconnected traders
	int num_disconnected = 0;

	while(1) {
		int disconnect_return = check_disconnect(&trader_arr, num_traders, &num_disconnected, &exchange_fees);
		if (disconnect_return == 1) {
			//close fifos
			for(int i = 0 ; i < num_traders ; i++) {
				close(trader_arr[i].exchange_fd);
				close(trader_arr[i].trader_fd);
				char* trader_name = (char*) malloc(sizeof("/tmp/pe_trader_") + sizeof(int));
				get_fifo_trader(i, trader_name);
				unlink(trader_name);
				char* exchange_name = (char*) malloc(sizeof("/tmp/pe_exchange_") + sizeof(int));
				get_fifo_exchange(i, exchange_name);
				unlink(exchange_name);
				free(trader_name);
				free(exchange_name);
			}

			free_arrays(&buy_arr, &sell_arr, &trader_arr, &product_arr, buy_arr_i, sell_arr_i, num_traders, num_products);	

			return 0;
		}
		else if (disconnect_return == 2) {
			continue;
		}

		pause();

		if (signal_received == 1) {
			// find sending trader
			int trader_id;
			for(int i = 0 ; i < num_traders ; i++) {
				if(trader_arr[i].pid == sig_pid) {
					trader_id = i;
				}
			}

			// check if disconnected
			if (trader_arr[trader_id].disconnected == 1) {
				continue;
			}

			// read message
			char trader_msg[BUFFER];
			read(trader_arr[trader_id].trader_fd, trader_msg, BUFFER);
			

			struct ORDER order;
			order = parse_msg(trader_msg, trader_arr[trader_id], order_arr_i, &buy_arr, buy_arr_i, &product_arr, num_products, &trader_arr);

			switch (order.type) {
				case BUY:
					order_arr_i++;
					add_order_book(&order, &buy_arr, &buy_arr_i);
					send_accepted(&order, &trader_arr);
					broadcast_market(&order, &trader_arr, num_traders);
					match_buy(&buy_arr[buy_arr_i-1], &sell_arr, sell_arr_i, &exchange_fees, num_products, &trader_arr, num_traders);
					qsort((void*) buy_arr, buy_arr_i, sizeof(struct ORDER), comparator_buy);
					trader_arr[order.trader.id].current_order_id += 1;
					break;
				case SELL:
					order_arr_i++;
					add_order_book(&order, &sell_arr, &sell_arr_i);
					send_accepted(&order, &trader_arr);
					broadcast_market(&order, &trader_arr, num_traders);
					match_sell(&sell_arr[sell_arr_i-1], &buy_arr, buy_arr_i, &exchange_fees, num_products, &trader_arr, num_traders);
					qsort((void*) sell_arr, sell_arr_i, sizeof(struct ORDER), comparator_sell);
					trader_arr[order.trader.id].current_order_id += 1;
					break;
				case AMEND:
					int ret_value = amend_order(&order, &buy_arr, &sell_arr, buy_arr_i, sell_arr_i, &trader_arr, num_traders, trader_id, order_arr_i, &exchange_fees, num_products);
					if(ret_value == 1) {
						order.type = INVALID;
						send_invalid(&order);
						break;
					}
					order_arr_i++;
					
					qsort((void*) buy_arr, buy_arr_i, sizeof(struct ORDER), comparator_buy);
					qsort((void*) sell_arr, sell_arr_i, sizeof(struct ORDER), comparator_sell);
					break;
				case CANCEL:
					if (cancel_order(&order, &buy_arr, &sell_arr, buy_arr_i, sell_arr_i, &trader_arr, num_traders, trader_id) == 1) {
						order.type = INVALID;
						send_invalid(&order);
						break;
					};
					break;
				case INVALID:
					send_invalid(&order);
					break;
			}
			if (order.type != INVALID) {
				// print orderbook and positions
				print_orderbook(&buy_arr, &sell_arr, &product_arr, buy_arr_i, sell_arr_i, num_products);
				print_positions(&trader_arr, num_traders, num_products);
			}
			
			// reset signal flag
			signal_received = 0;

		}
	}

	//close fifos
	for(int i = 0 ; i < num_traders ; i++) {
		close(trader_arr[i].exchange_fd);
		close(trader_arr[i].trader_fd);
	}

	free_arrays(&buy_arr, &sell_arr, &trader_arr, &product_arr, buy_arr_i, sell_arr_i, num_traders, num_products);
}