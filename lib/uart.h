#ifndef UART_H
#define UART_H
#include <stdint.h>
void uart_init(void);
void uart_tx(uint8_t c);
uint8_t uart_rx(uint8_t *c);
#endif