#ifndef UART_H
#define UART_H
#include <stdint.h>
void uart_init(void);
int uart_tx_ready(void);
void uart_tx(uint8_t c);
int uart_rx(uint8_t *c);
#endif
