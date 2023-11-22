#include "data.h"
#include "midi.h"
#include "pin.h"
#include <avr/io.h>
#include <util/delay.h>

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

uint8_t uart_read(uint8_t *c) {
	uint8_t status;

	do
		status = UCSR1A;
	while (!(status & _BV(RXC1)));
	*c = UDR1;
	return !(status & (_BV(FE1) | _BV(DOR1) | _BV(UPE1)));
}

pin yellow = {&PORTB, &DDRB, PORTB0};

int main(void) {
	int i;
	uart_init();

	for (i = 0; i < (int)sizeof(data); i++) {
		unsigned char c = data[i];
		if (c == 0xf8)
			_delay_ms(21); /* MIDI clock 118 BPM, 24 PPQ */
		uart_tx(data[i]);
	}
	while (1)
		;
}
