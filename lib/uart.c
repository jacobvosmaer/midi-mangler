/* MIDI uart function */
#include "uart.h"
#include <avr/io.h>
void uart_init(void) {
	UBRR1 = (F_CPU / 16 / 31250) - 1;
	UCSR1B = 1 << RXEN1 | 1 << TXEN1;
}
int uart_tx_ready(void) { return UCSR1A & (1 << UDRE1); }
void uart_tx(uint8_t c) {
	while (!uart_tx_ready())
		;
	UDR1 = c;
}
int uart_rx(uint8_t *c) {
	uint8_t status = UCSR1A;
	if (!(status & 1 << RXC1))
		return 0;
	*c = UDR1;
	return !(status & (1 << FE1 | 1 << DOR1 | 1 << UPE1));
}
