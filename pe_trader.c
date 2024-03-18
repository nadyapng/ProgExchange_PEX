#include "pe_trader.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include<sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>

#define MAX_WORDS 6

// signal received bool
int signal_received = 0;

int check_trader_id(char arg[]) {
    // check negative 
    if (arg[0] == '-') {
        return 0;
    }
    else {
        for (int i = 0 ; arg[i] != 0 ; i++) {
            if(isdigit(arg[i]) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

void get_fifo_exchange(int trader_id, char *name) {
    sprintf(name, FIFO_EXCHANGE, trader_id);
}

void get_fifo_trader(int trader_id, char *name) {
    sprintf(name, FIFO_TRADER, trader_id);
}

int parse_msg(struct ORDER_T *order, char *msg) {
    // strip end ;
    msg[strcspn(msg, ";")] = 0;
    // split command

    char *ptr = strtok(msg, " ");
    if (strcmp(ptr, "MARKET") == 0) {
        // printf("%s\n", strtok(NULL, " "));
        order->type = strtok(NULL, " ");
        order->product = strtok(NULL, " ");
        // TODO check if ints are not chars
        order->qty = atoi(strtok(NULL, " "));
        order->price = atoi(strtok(NULL, " "));
        return 0;
    }
    else if ((strcmp(ptr, "ACCEPTED") == 0)| (strcmp(ptr, "AMENDED") == 0)| (strcmp(ptr, "CANCELLED") == 0)){
        return 2;
    }
    else if (strcmp(ptr, "FILL") == 0) {
        return 3;
    }

    return 1;
}

void send_auto_order(struct ORDER_T *order, int id, int fd_trader) {
    // generate order
    char new_msg[128];
    sprintf(new_msg, "%s %d %s %d %d;", "BUY", id, order->product, order->qty, order->price);
    write(fd_trader, new_msg, sizeof(new_msg));
}


// sig handler
void handle_sigusr(int sig) {
    signal_received = 1;
}


int main(int argc, char ** argv) {
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    // get trader id
    // check if trader id is valid
    if(check_trader_id(argv[1]) == 0) {
        printf("Invalid trader id\n");
        return 1;
    }

    int trader_id = atoi(argv[1]);

    // register signal handler
    struct sigaction sa;
    sa.sa_handler = &handle_sigusr;
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    // connect to named pipes
    // connect to pe_exchange
    // get fifo name with trader id
    char fifo_name_exchange[100];
    get_fifo_exchange(trader_id, fifo_name_exchange);
    int fd_exchange = open(fifo_name_exchange, O_RDONLY);
    // check for errors
    if (fd_exchange == -1) {
        printf("Cannot connect to exchange fifo\n");
        return 2;
    }
    
    // connect to pe_trader
    char fifo_name_trader[100];
    get_fifo_trader(trader_id, fifo_name_trader);
    int fd_trader = open(fifo_name_trader, O_WRONLY);
    if(fd_trader == -1) {
        printf("Cannot connect to trader fifo\n");
        return 3;
    }

    // wait for market open message
    pause();
    char open_msg[128];
    read(fd_exchange, open_msg, 128);

    // initiate order id
    int order_id = 0;

    // event loop:
    while(1) {
        // reset signal_received bool
        signal_received = 0;
        // wait for exchange update (MARKET message)
        pause();
        char market_msg[128];
        read(fd_exchange, market_msg, 128);


        // new msg struct
        struct ORDER_T order;
        // parse msg
        if ((parse_msg(&order, market_msg) == 0) && (strcmp(order.type, "SELL") == 0)) {

            // check qty 
            if (order.qty >= 1000) {
                close(fd_trader);
                close(fd_exchange);
                return 1;
            }

            // send buy message
            char new_msg[128];
            sprintf(new_msg, "%s %d %s %d %d;", "BUY", order_id, order.product, order.qty, order.price);
            write(fd_trader, new_msg, strlen(new_msg));


            // increment order id
            order_id++;
            
            // send signal 
            kill(getppid(), SIGUSR1);
        }
    }

    // close fifos
    close(fd_trader);
    close(fd_exchange);

}