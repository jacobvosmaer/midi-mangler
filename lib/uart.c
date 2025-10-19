/* MIDI uart function */
#include "uart.h"
#include <avr/io.h>
void uart_init(void) {
	enum { midi_baud = 31250, ubrr = (F_CPU / 16 / midi_baud) - 1 };
	UBRR1H = ubrr >> 8;
	UBRR1L = ubrr & 0xff;
	UCSR1B = _BV(RXEN1) | _BV(TXEN1);
}
void uart_tx(uint8_t c) {
	while (!(UCSR1A & (1 << UDRE1)))
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
