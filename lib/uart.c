/* MIDI uart function */
#include "uart.h"
#include <avr/io.h>
void uart_init(void) {
	UBRR1 = (F_CPU / 16 / 31250) - 1;
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
}
uint8_t uart_tx_ready(void) { return UCSR1A & (1 << UDRE1); }
void uart_tx(uint8_t c) {
	while (!uart_tx_ready())
		;
	UDR1 = c;
}
uint8_t uart_rx(uint8_t *c) {
	uint8_t status = UCSR1A;
	if (!(status & _BV(RXC1)))
		return 0;
	*c = UDR1;
	return !(status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1)));
}
